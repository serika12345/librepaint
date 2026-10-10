/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentRecoveryFile.h"
#include <QCryptographicHash>
#include <QFile>
#include <QSaveFile>
#include <QtEndian>
#include <cstring>
#include <limits>
#include <map>

namespace {
using File = KisGpuDocumentRecoveryFile;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
using Store = KisGpuTileStore;
using Error = File::Error;
constexpr quint32 Absent = std::numeric_limits<quint32>::max();
constexpr qsizetype HeaderBytes = 52;
constexpr quint32 CurrentVersion = 2;
struct Failure { Error error; };
void require(bool condition, Error error = Error::InvalidData) { if (!condition) throw Failure{error}; }

struct Writer {
    QByteArray bytes;
    quint64 maximum;
    void put(const char *data, qsizetype size) {
        require(quint64(size) <= maximum - quint64(bytes.size()), Error::BudgetExceeded);
        bytes.append(data, size);
    }
    template<typename T> void number(T value) {
        char data[sizeof(T)];
        qToBigEndian<T>(value, data);
        put(data, sizeof(T));
    }
    void real(double value) {
        static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);
        quint64 bits;
        std::memcpy(&bits, &value, 8);
        number(bits);
    }
    void count(qsizetype value) {
        require(value >= 0 && quint64(value) < Absent, Error::BudgetExceeded);
        number(quint32(value));
    }
    void raw(const QByteArray &data) { number(quint64(data.size())); put(data.constData(), data.size()); }
    void rectangle(QRect value) {
        number(qint32(value.x())); number(qint32(value.y()));
        number(qint32(value.width())); number(qint32(value.height()));
    }
    void name(const QString &value) {
        count(value.size());
        for (const auto character : value) number(quint16(character.unicode()));
    }
};
struct MemoryBudget {
    quint64 maximum, allocated = 0;
    void reserve(quint64 size) {
        require(size <= maximum - allocated, Error::BudgetExceeded);
        allocated += size;
    }
};
struct Reader {
    const QByteArray &bytes;
    MemoryBudget memory;
    qsizetype position = 0;
    quint64 remaining() const { return quint64(bytes.size() - position); }
    template<typename T> T number() {
        require(remaining() >= sizeof(T));
        const auto value = qFromBigEndian<T>(bytes.constData() + position);
        position += sizeof(T);
        return value;
    }
    bool boolean() { const auto value = number<quint8>(); require(value <= 1); return value; }
    double real() {
        const auto bits = number<quint64>();
        double value;
        std::memcpy(&value, &bits, 8);
        return value;
    }
    quint32 count(quint64 minimumBytes, quint64 itemBytes) {
        const auto value = number<quint32>();
        require(value <= remaining() / minimumBytes);
        memory.reserve(quint64(value) * itemBytes);
        return value;
    }
    QByteArray raw() {
        const auto size = number<quint64>();
        require(size <= remaining());
        memory.reserve(size);
        auto result = bytes.mid(position, qsizetype(size));
        position += qsizetype(size);
        return result;
    }
    QRect rectangle() {
        const auto x = number<qint32>(), y = number<qint32>();
        const auto width = number<qint32>(), height = number<qint32>();
        return {x, y, width, height};
    }
    QString name() {
        const auto size = count(2, sizeof(QChar));
        QString result;
        result.resize(size);
        for (quint32 i = 0; i < size; ++i) result[i] = QChar(number<quint16>());
        return result;
    }
};
template<typename T> struct Table {
    QVector<std::shared_ptr<const T>> values;
    std::map<const T *, quint32> indices;
    void add(const std::shared_ptr<const T> &value) {
        if (value && !indices.count(value.get())) {
            require(quint64(values.size()) < Absent, Error::BudgetExceeded);
            indices[value.get()] = quint32(values.size());
            values.push_back(value);
        }
    }
    quint32 index(const std::shared_ptr<const T> &value) const { return value ? indices.at(value.get()) : Absent; }
};
struct Tables {
    Table<Session::ReplaySelection> selections;
    Table<Session::ReplayTexture> textures;
    Table<Session::Recovery> rasters;
    Table<Document::RecoveryLayer> layers;
    void assets(const Session::Recovery &data) {
        const auto edit = [&](const Session::ReplayEdit &edit) {
            for (const auto &batch : edit) { selections.add(batch.selection); textures.add(batch.texture); }
        };
        for (const auto &commands : data.edits) edit(commands);
        if (data.working) edit(*data.working);
    }
    explicit Tables(const Document::Recovery &data) {
        for (const auto &frame : data.history) for (const auto &layer : frame) layers.add(layer);
        for (const auto &layer : layers.values) { rasters.add(layer->pixels); rasters.add(layer->mask); }
        for (const auto &raster : rasters.values) assets(*raster);
        if (data.working) assets(data.working->data);
    }
};
void writeCommands(Writer &out, const QVector<Store::DabCommand> &commands)
{
    out.count(commands.size());
    for (const auto &command : commands) {
        out.real(command.center.x()); out.real(command.center.y());
        out.real(command.diameter.width()); out.real(command.diameter.height());
        out.real(command.fade.width()); out.real(command.fade.height());
        out.number(command.rgba); out.number(quint8(command.operation == Store::CompositeOp::Erase ? 1 : 0));
        out.number(command.opacity); out.number(command.coverage); out.number(command.textureStrength);
    }
}
QVector<Store::DabCommand> readCommands(Reader &in)
{
    const auto count = in.count(56, sizeof(Store::DabCommand));
    QVector<Store::DabCommand> result;
    result.reserve(count);
    for (quint32 i = 0; i < count; ++i) {
        Store::DabCommand command;
        const auto x = in.real(), y = in.real();
        command.center = {x, y};
        const auto width = in.real(), height = in.real();
        command.diameter = {width, height};
        const auto fx = in.real(), fy = in.real();
        command.fade = {fx, fy};
        command.rgba = in.number<quint32>();
        const auto operation = in.number<quint8>();
        require(operation <= 1);
        command.operation = operation ? Store::CompositeOp::Erase : Store::CompositeOp::Over;
        command.opacity = in.number<quint8>(); command.coverage = in.number<quint8>();
        command.textureStrength = in.number<quint8>();
        result.push_back(command);
    }
    return result;
}
template<typename T> std::shared_ptr<const T> reference(Reader &in, const QVector<std::shared_ptr<const T>> &table,
                                                       bool nullable = true)
{
    const auto index = in.number<quint32>();
    if (nullable && index == Absent) return {};
    require(index < quint64(table.size()));
    return table[index];
}
void writeEdit(Writer &out, const Session::ReplayEdit &edit, const Tables &tables)
{
    out.count(edit.size());
    for (const auto &batch : edit) {
        writeCommands(out, batch.commands); out.rectangle(batch.clip);
        out.number(tables.selections.index(batch.selection)); out.number(tables.textures.index(batch.texture));
    }
}
Session::ReplayEdit readEdit(Reader &in, const Tables &tables)
{
    const auto count = in.count(28, sizeof(Session::ReplayBatch));
    Session::ReplayEdit result;
    result.reserve(count);
    for (quint32 i = 0; i < count; ++i) {
        Session::ReplayBatch batch;
        batch.commands = readCommands(in); batch.clip = in.rectangle();
        batch.selection = reference(in, tables.selections.values); batch.texture = reference(in, tables.textures.values);
        result.push_back(std::move(batch));
    }
    return result;
}
void writeRaster(Writer &out, const Session::Recovery &data, const Tables &tables)
{
    out.rectangle(data.bounds); out.raw(data.pixels); out.count(data.tiles.size());
    for (const auto &tile : data.tiles) {
        out.number(qint32(tile.coordinate.x())); out.number(qint32(tile.coordinate.y())); out.raw(tile.pixels);
    }
    out.count(data.edits.size());
    for (const auto &edit : data.edits) writeEdit(out, edit, tables);
    out.number(qint64(data.firstRetained)); out.number(qint64(data.cursor));
    out.number(quint8(data.working.has_value()));
    if (data.working) writeEdit(out, *data.working, tables);
    out.number(quint8(data.commitRequested));
}
Session::Recovery readRaster(Reader &in, const Tables &tables)
{
    Session::Recovery data;
    data.bounds = in.rectangle(); data.pixels = in.raw();
    const auto tiles = in.count(16, sizeof(Session::RecoveryTile));
    data.tiles.reserve(tiles);
    for (quint32 i = 0; i < tiles; ++i) {
        const auto x = in.number<qint32>(), y = in.number<qint32>();
        data.tiles.push_back({{x, y}, in.raw()});
    }
    const auto edits = in.count(4, sizeof(Session::ReplayEdit));
    data.edits.reserve(edits);
    for (quint32 i = 0; i < edits; ++i) data.edits.push_back(readEdit(in, tables));
    const auto first = in.number<qint64>(), cursor = in.number<qint64>();
    require(first >= 0 && cursor >= first && quint64(cursor) <= quint64(data.edits.size()));
    data.firstRetained = qsizetype(first); data.cursor = qsizetype(cursor);
    if (in.boolean()) data.working = readEdit(in, tables);
    data.commitRequested = in.boolean();
    return data;
}
void checkRecovery(const Document::Recovery &data, quint64 maximum)
{
    if (!Document::validRecovery(data, data.history.size() - 1, maximum)) {
        throw Failure{Document::validRecovery(data, data.history.size() - 1, std::numeric_limits<quint64>::max())
            ? Error::BudgetExceeded : Error::InvalidData};
    }
}
void checkReadBudget(const Document::Recovery &data, const Tables &tables, quint64 maximum)
{
    MemoryBudget memory{maximum};
    const auto commands = [&](const QVector<Store::DabCommand> &values) {
        memory.reserve(quint64(values.size()) * sizeof(Store::DabCommand));
    };
    const auto edit = [&](const Session::ReplayEdit &values) {
        memory.reserve(quint64(values.size()) * sizeof(Session::ReplayBatch));
        for (const auto &batch : values) commands(batch.commands);
    };
    const auto raster = [&](const Session::Recovery &value) {
        memory.reserve(value.pixels.size());
        memory.reserve(quint64(value.tiles.size()) * sizeof(Session::RecoveryTile));
        for (const auto &tile : value.tiles) memory.reserve(tile.pixels.size());
        memory.reserve(quint64(value.edits.size()) * sizeof(Session::ReplayEdit));
        for (const auto &values : value.edits) edit(values);
        if (value.working) edit(*value.working);
    };
    for (const auto &selection : tables.selections.values) {
        memory.reserve(sizeof(Session::ReplaySelection) + sizeof(std::shared_ptr<const Session::ReplaySelection>));
        commands(selection->commands);
    }
    for (const auto &texture : tables.textures.values)
        memory.reserve(sizeof(Session::ReplayTexture) + sizeof(std::shared_ptr<const Session::ReplayTexture>) + quint64(texture->alpha.size()));
    for (const auto &value : tables.rasters.values) {
        memory.reserve(sizeof(Session::Recovery) + sizeof(Document::RasterRecovery));
        raster(*value);
    }
    for (const auto &layer : tables.layers.values)
        memory.reserve(sizeof(Document::RecoveryLayer) + sizeof(std::shared_ptr<const Document::RecoveryLayer>)
                       + quint64(layer->name.size()) * sizeof(QChar));
    memory.reserve(quint64(data.history.size()) * sizeof(Document::RecoveryFrame));
    for (const auto &frame : data.history) memory.reserve(quint64(frame.size()) * sizeof(std::shared_ptr<const Document::RecoveryLayer>));
    if (data.working) raster(data.working->data);
}
QByteArray encode(const Document::Recovery &data, quint64 maximum)
{
    checkRecovery(data, maximum);
    const Tables tables(data);
    checkReadBudget(data, tables, maximum);
    Writer out{{}, maximum};
    out.count(tables.selections.values.size());
    for (const auto &selection : tables.selections.values) {
        writeCommands(out, selection->commands); out.rectangle(selection->clip);
    }
    out.count(tables.textures.values.size());
    for (const auto &texture : tables.textures.values) {
        out.number(qint32(texture->size.width())); out.number(qint32(texture->size.height()));
        out.number(qint32(texture->origin.x())); out.number(qint32(texture->origin.y())); out.raw(texture->alpha);
    }
    out.count(tables.rasters.values.size());
    for (const auto &raster : tables.rasters.values) writeRaster(out, *raster, tables);
    out.count(tables.layers.values.size());
    for (const auto &layer : tables.layers.values) {
        out.number(layer->id); out.name(layer->name); out.number(layer->opacity); out.number(quint8(layer->operation == Store::CompositeOp::Erase ? 1 : 0));
        out.number(quint8(layer->visible)); out.number(quint8(layer->locked));
        out.number(tables.rasters.index(layer->pixels)); out.number(tables.rasters.index(layer->mask));
    }
    out.count(data.history.size());
    for (const auto &frame : data.history) {
        out.count(frame.size());
        for (const auto &layer : frame) out.number(tables.layers.index(layer));
    }
    out.number(qint64(data.cursor)); out.number(data.nextId); out.number(quint8(data.working.has_value()));
    if (data.working) {
        out.number(tables.layers.index(data.working->base)); out.number(quint8(data.working->target == Document::Target::Mask ? 1 : 0));
        out.number(quint8(data.working->completed)); writeRaster(out, data.working->data, tables);
    }
    return out.bytes;
}
Document::Recovery decode(const QByteArray &bytes, quint64 maximum, quint32 version)
{
    Reader in{bytes, {maximum}};
    Tables tables(Document::Recovery{});
    const auto selections = in.count(20, sizeof(Session::ReplaySelection) + sizeof(std::shared_ptr<const Session::ReplaySelection>));
    tables.selections.values.reserve(selections);
    for (quint32 i = 0; i < selections; ++i) {
        auto value = std::make_shared<Session::ReplaySelection>();
        value->commands = readCommands(in); value->clip = in.rectangle();
        tables.selections.values.push_back(std::move(value));
    }
    const auto textures = in.count(24, sizeof(Session::ReplayTexture) + sizeof(std::shared_ptr<const Session::ReplayTexture>));
    tables.textures.values.reserve(textures);
    for (quint32 i = 0; i < textures; ++i) {
        auto value = std::make_shared<Session::ReplayTexture>();
        const auto width = in.number<qint32>(), height = in.number<qint32>();
        value->size = {width, height};
        const auto x = in.number<qint32>(), y = in.number<qint32>();
        value->origin = {x, y}; value->alpha = in.raw();
        tables.textures.values.push_back(std::move(value));
    }
    const auto rasters = in.count(50, sizeof(Session::Recovery) + sizeof(Document::RasterRecovery));
    tables.rasters.values.reserve(rasters);
    for (quint32 i = 0; i < rasters; ++i) tables.rasters.values.push_back(std::make_shared<const Session::Recovery>(readRaster(in, tables)));
    const auto layers = in.count(version == 1 ? 22 : 24, sizeof(Document::RecoveryLayer) + sizeof(std::shared_ptr<const Document::RecoveryLayer>));
    tables.layers.values.reserve(layers);
    for (quint32 i = 0; i < layers; ++i) {
        auto value = std::make_shared<Document::RecoveryLayer>();
        value->id = in.number<quint64>(); value->name = in.name(); value->opacity = in.number<quint8>();
        const auto operation = in.number<quint8>(); require(operation <= 1);
        value->operation = operation ? Store::CompositeOp::Erase : Store::CompositeOp::Over;
        if (version >= 2) { value->visible = in.boolean(); value->locked = in.boolean(); }
        value->pixels = reference(in, tables.rasters.values, false); value->mask = reference(in, tables.rasters.values);
        tables.layers.values.push_back(std::move(value));
    }
    Document::Recovery result;
    const auto frames = in.count(4, sizeof(Document::RecoveryFrame));
    result.history.reserve(frames);
    for (quint32 i = 0; i < frames; ++i) {
        Document::RecoveryFrame frame;
        const auto count = in.count(4, sizeof(std::shared_ptr<const Document::RecoveryLayer>));
        frame.reserve(count);
        for (quint32 j = 0; j < count; ++j) frame.push_back(reference(in, tables.layers.values, false));
        result.history.push_back(std::move(frame));
    }
    const auto cursor = in.number<qint64>();
    require(cursor >= 0 && quint64(cursor) < quint64(result.history.size()));
    result.cursor = qsizetype(cursor); result.nextId = in.number<quint64>();
    if (in.boolean()) {
        Document::RecoveryWorking working;
        working.base = reference(in, tables.layers.values, false);
        const auto target = in.number<quint8>(); require(target <= 1);
        working.target = target ? Document::Target::Mask : Document::Target::Pixels; working.completed = in.boolean();
        working.data = readRaster(in, tables);
        result.working = std::move(working);
    }
    require(!in.remaining());
    checkRecovery(result, maximum);
    return result;
}
}

File::Error File::write(const QString &path, const Document::Recovery &data, quint64 maximum)
{
    try {
        const auto payload = encode(data, maximum);
        Writer header{{}, HeaderBytes};
        header.put("LPGPUREC", 8); header.number(CurrentVersion); header.number(quint64(payload.size()));
        const auto digest = QCryptographicHash::hash(payload, QCryptographicHash::Sha256);
        header.put(digest.constData(), digest.size());
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(header.bytes) != HeaderBytes
            || file.write(payload) != payload.size() || !file.commit()) return Error::Io;
        return Error::None;
    } catch (const Failure &failure) { return failure.error; }
}

File::ReadResult File::read(const QString &path, quint64 maximum)
{
    try {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return {Error::Io, {}};
        require(file.size() >= HeaderBytes);
        require(quint64(file.size() - HeaderBytes) <= maximum, Error::BudgetExceeded);
        const auto header = file.read(HeaderBytes);
        require(header.size() == HeaderBytes, file.error() == QFile::NoError ? Error::InvalidData : Error::Io);
        require(header.first(8) == QByteArray("LPGPUREC", 8));
        Reader input{header, {HeaderBytes}}; input.position = 8;
        const auto version = input.number<quint32>();
        require(version >= 1 && version <= CurrentVersion, Error::UnsupportedVersion);
        const auto size = input.number<quint64>();
        require(size == quint64(file.size() - HeaderBytes));
        const auto payload = file.read(qint64(size));
        require(quint64(payload.size()) == size, file.error() == QFile::NoError ? Error::InvalidData : Error::Io);
        require(QCryptographicHash::hash(payload, QCryptographicHash::Sha256) == header.mid(20, 32));
        return {Error::None, decode(payload, maximum, version)};
    } catch (const Failure &failure) { return {failure.error, {}}; }
}
