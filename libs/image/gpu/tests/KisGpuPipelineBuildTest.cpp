/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QTest>
#include <vector>
#include "../KisGpuTileStorage_p.h"

namespace LibrePaint::Image::Gpu {
class PipelineCompilationProbe
{
public:
    static Detail::NativeComputePipeline compile(TileRenderer &renderer, const QString &path, const QByteArray &entry,
        quint32 first = 0, quint32 second = 0, quint32 format = 0, bool generated = false)
    { return renderer.m_d->createPipeline(path, entry.constData(), nullptr, first, second, format, generated); }
    static QString error(TileRenderer &renderer) { return renderer.m_d->error; }
};
}
using namespace LibrePaint::Image::Gpu;

class KisGpuPipelineBuildTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void allProgramsCompileWithinSecondsAndReuseTheirCache()
    {
        QString diagnostic;
        auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        QElapsedTimer timer;
        timer.start();
        TileRenderer renderer(device);
        qInfo() << "Initial paint/projection pipelines" << timer.elapsed() << "ms";
        struct Program { QString path; QByteArray entry; Detail::NativeComputePipeline pipeline;
            quint32 first = 0, second = 0, format = 0; bool generated = false; };
        std::vector<Program> programs;
        const QString prefix = QStringLiteral(":/librepaint/gpu/shaders/");
        const QRegularExpression compute(QStringLiteral("@compute\\s+@workgroup_size\\([^)]*\\)\\s*fn\\s+(\\w+)\\s*\\("));
        for (const auto &name : QDir(prefix).entryList({QStringLiteral("*.wgsl")}, QDir::Files)) {
            const auto path = prefix + name;
            QFile file(path);
            QVERIFY(file.open(QIODevice::ReadOnly));
            const auto source = QString::fromUtf8(file.readAll());
            auto entries = compute.globalMatch(source);
            int count = 0;
            while (entries.hasNext()) {
                const auto entry = entries.next().captured(1).toUtf8();
                QElapsedTimer compilation; compilation.start();
                auto pipeline = PipelineCompilationProbe::compile(renderer, path, entry);
                if (compilation.elapsed() > 250) qInfo() << name << entry << compilation.elapsed() << "ms";
                QVERIFY2(pipeline, qPrintable(path + QStringLiteral(": ") + PipelineCompilationProbe::error(renderer)));
                programs.push_back({path, entry, std::move(pipeline)});
                ++count;
            }
            QCOMPARE(count, source.count(QStringLiteral("@compute")));
        }
        const auto painting = prefix + QStringLiteral("paint.wgsl");
        auto mixed = PipelineCompilationProbe::compile(renderer, painting, "main", 60, 88, 2, true);
        QVERIFY2(mixed, qPrintable(PipelineCompilationProbe::error(renderer)));
        programs.push_back({painting, "main", std::move(mixed), 60, 88, 2, true});
        auto coupled = PipelineCompilationProbe::compile(renderer, painting, "main", 88, 0, 1, false);
        QVERIFY2(coupled, qPrintable(PipelineCompilationProbe::error(renderer)));
        programs.push_back({painting, "main", std::move(coupled), 88, 0, 1, false});
        QVERIFY(programs.size() >= 20);
        const auto cold = timer.elapsed();
        QVERIFY2(cold < 8000, qPrintable(QStringLiteral("All %1 pipelines took %2 ms").arg(programs.size()).arg(cold)));
        timer.restart();
        for (const auto &program : programs) {
            const auto cached = PipelineCompilationProbe::compile(renderer, program.path, program.entry, program.first,
                program.second, program.format, program.generated);
            QCOMPARE(cached.get(), program.pipeline.get());
        }
        qInfo() << programs.size() << "pipelines; cold" << cold << "ms; cached" << timer.elapsed() << "ms";
    }
};

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuPipelineBuildTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPipelineBuildTest.moc"

