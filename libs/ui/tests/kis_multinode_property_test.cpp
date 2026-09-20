/*
 * SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QHash>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include <type_traits>

#include "nodes/kis_multinode_property.h"

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

namespace
{

const QString normalCompositeOp = QStringLiteral("normal");
const QString darkenCompositeOp = QStringLiteral("alphadarken");

struct NodeState {
    QString compositeOp{normalCompositeOp};
    QString name;
    int colorLabel{0};
    quint8 opacity{255};
    KisBaseNode::PropertyList properties;
    bool layer{true};
    const KoColorSpace *colorSpace{nullptr};
    QStringList channelNames;
    QBitArray channelFlags;
};

QHash<const KisNode *, NodeState> nodeStates;

bool isBoolean(const QVariant &value)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return value.metaType().id() == QMetaType::Bool;
#else
    return value.type() == QVariant::Bool;
#endif
}

KisBaseNode::Property makeProperty(const QString &id, const QString &name, const QVariant &state)
{
    KisBaseNode::Property result;
    result.id = id;
    result.name = name;
    result.state = state;
    result.isMutable = isBoolean(state);
    return result;
}

KisNodeSP node(quintptr id,
               const QString &name,
               const KoColorSpace *colorSpace = reinterpret_cast<const KoColorSpace *>(quintptr(1)))
{
    auto *pointer = reinterpret_cast<KisNode *>(id * 16 + 1);
    NodeState state;
    state.name = name;
    state.colorSpace = colorSpace;
    state.channelNames = {QStringLiteral("Red"),
                          QStringLiteral("Green"),
                          QStringLiteral("Blue"),
                          QStringLiteral("Alpha")};
    state.properties = {makeProperty(QStringLiteral("visible"), QStringLiteral("Visible"), true),
                        makeProperty(QStringLiteral("locked"), QStringLiteral("Locked"), false),
                        makeProperty(QStringLiteral("opacity"), QStringLiteral("Opacity"), QStringLiteral("100%"))};
    nodeStates.insert(pointer, state);
    return KisNodeSP(pointer);
}

NodeState &state(const KisNodeSP &node)
{
    return nodeStates[node.data()];
}

const NodeState &constState(const KisNodeSP &node)
{
    return nodeStates[node.data()];
}

class LayerFixture
{
public:
    LayerFixture()
        : first(node(1, QStringLiteral("Layer 1")))
        , second(node(2, QStringLiteral("Layer 2")))
        , third(node(3, QStringLiteral("Layer 3")))
    {
    }

    KisNodeList nodes() const
    {
        return {first, second, third};
    }

    KisNodeSP first;
    KisNodeSP second;
    KisNodeSP third;
};

KisBaseNode::Property firstBooleanProperty(const KisNodeSP &node)
{
    for (const KisBaseNode::Property &item : constState(node).properties) {
        if (isBoolean(item.state)) {
            return item;
        }
    }
    return {};
}

class FakeProperty final : public KisMultinodePropertyInterface
{
public:
    void rereadCurrentValue() override
    {
        ++rereadCount;
    }

    void setIgnored(bool value) override
    {
        ignored = value;
        ++setIgnoredCount;
    }

    bool isIgnored() const override
    {
        return ignored;
    }

    bool savedValuesDiffer() const override
    {
        return valuesDiffer;
    }

    bool haveTheOnlyNode() const override
    {
        return onlyNode;
    }

    void connectValueChangedSignal(const QObject *, const char *, Qt::ConnectionType) override
    {
    }

    void connectIgnoreCheckBox(QCheckBox *) override
    {
    }

    void connectAutoEnableWidget(QWidget *) override
    {
    }

    KUndo2Command *createPostExecutionUndoCommand() override
    {
        return new KUndo2Command();
    }

    bool ignored{true};
    bool valuesDiffer{true};
    bool onlyNode{false};
    int rereadCount{0};
    int setIgnoredCount{0};
};

} // namespace

QString BaseAdapter::NodeAccess::compositeOpId(KisNodeSP node)
{
    return constState(node).compositeOp;
}

void BaseAdapter::NodeAccess::setCompositeOpId(KisNodeSP node, const QString &value)
{
    state(node).compositeOp = value;
}

QString BaseAdapter::NodeAccess::name(KisNodeSP node)
{
    return constState(node).name;
}

void BaseAdapter::NodeAccess::setName(KisNodeSP node, const QString &value)
{
    state(node).name = value;
}

int BaseAdapter::NodeAccess::colorLabelIndex(KisNodeSP node)
{
    return constState(node).colorLabel;
}

void BaseAdapter::NodeAccess::setColorLabelIndex(KisNodeSP node, int value)
{
    state(node).colorLabel = value;
}

quint8 BaseAdapter::NodeAccess::opacity(KisNodeSP node)
{
    return constState(node).opacity;
}

void BaseAdapter::NodeAccess::setOpacity(KisNodeSP node, quint8 value)
{
    state(node).opacity = value;
}

KisBaseNode::PropertyList BaseAdapter::NodeAccess::sectionModelProperties(KisNodeSP node)
{
    return constState(node).properties;
}

void BaseAdapter::NodeAccess::setSectionModelProperties(KisNodeSP node, const KisBaseNode::PropertyList &properties)
{
    state(node).properties = properties;
}

bool BaseAdapter::NodeAccess::isLayer(KisNodeSP node)
{
    return constState(node).layer;
}

const KoColorSpace *BaseAdapter::NodeAccess::colorSpace(KisNodeSP node)
{
    return constState(node).colorSpace;
}

QStringList BaseAdapter::NodeAccess::channelNames(KisNodeSP node)
{
    return constState(node).channelNames;
}

int BaseAdapter::NodeAccess::channelCount(KisNodeSP node)
{
    return constState(node).channelNames.size();
}

QBitArray BaseAdapter::NodeAccess::channelFlags(KisNodeSP node)
{
    return constState(node).channelFlags;
}

void BaseAdapter::NodeAccess::setChannelFlags(KisNodeSP node, const QBitArray &flags)
{
    state(node).channelFlags = flags;
}

class KisMultinodePropertyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void baseAndScalarAdaptersRoundTrip();
    void layerPropertyAdaptersDiscoverAndChangeBooleanProperties();
    void channelFlagAdaptersRequireCompatibleLayers();
    void multinodePropertyTracksUniformIgnoredAndRereadStates();
    void connectorWidgetsAndSignalsFollowIgnoreState();
    void boolConnectorMapsTriStateToProperty();
    void undoCommandsRestoreAndReapplyPerNodeValues();
    void recordValueChanged();

private:
    int m_valueChangedCount{0};
};

void KisMultinodePropertyTest::init()
{
    nodeStates.clear();
    m_valueChangedCount = 0;
}

void KisMultinodePropertyTest::baseAndScalarAdaptersRoundTrip()
{
    static_assert(std::is_same_v<CompositeOpAdapter::ValueType, QString>);
    static_assert(std::is_same_v<CompositeOpAdapter::ConnectorType, MultinodePropertyBaseConnector>);
    static_assert(std::is_same_v<NameAdapter::ValueType, QString>);
    static_assert(std::is_same_v<NameAdapter::ConnectorType, MultinodePropertyBaseConnector>);
    static_assert(std::is_same_v<ColorLabelAdapter::ValueType, int>);
    static_assert(std::is_same_v<ColorLabelAdapter::ConnectorType, MultinodePropertyBaseConnector>);
    static_assert(std::is_same_v<OpacityAdapter::ValueType, int>);
    static_assert(std::is_same_v<OpacityAdapter::ConnectorType, MultinodePropertyBaseConnector>);
    static_assert(!CompositeOpAdapter::forceIgnoreByDefault);
    static_assert(NameAdapter::forceIgnoreByDefault);
    static_assert(!ColorLabelAdapter::forceIgnoreByDefault);
    static_assert(!OpacityAdapter::forceIgnoreByDefault);

    LayerFixture fixture;
    BaseAdapter base;
    QCOMPARE(BaseAdapter::filterNodes(fixture.nodes()), fixture.nodes());
    base.setNumNodes(3);
    QCOMPARE(base.m_numNodes, 3);

    CompositeOpAdapter::setPropForNode(fixture.first, darkenCompositeOp, 17);
    QCOMPARE(CompositeOpAdapter::propForNode(fixture.first), darkenCompositeOp);

    NameAdapter name;
    name.setNumNodes(3);
    QCOMPARE(name.propForNode(fixture.first), QStringLiteral("Layer"));
    name.setPropForNode(fixture.first, QStringLiteral("Renamed 82"), 2);
    QCOMPARE(constState(fixture.first).name, QStringLiteral("Renamed 2"));
    name.setPropForNode(fixture.first, QStringLiteral("Exact 82"), -1);
    QCOMPARE(constState(fixture.first).name, QStringLiteral("Exact 82"));
    name.setNumNodes(1);
    QCOMPARE(name.propForNode(fixture.first), QStringLiteral("Exact 82"));

    ColorLabelAdapter::setPropForNode(fixture.second, 7, 3);
    QCOMPARE(ColorLabelAdapter::propForNode(fixture.second), 7);

    OpacityAdapter::setPropForNode(fixture.third, 50, 9);
    QCOMPARE(constState(fixture.third).opacity, quint8(128));
    QCOMPARE(OpacityAdapter::propForNode(fixture.third), 50);
}

void KisMultinodePropertyTest::layerPropertyAdaptersDiscoverAndChangeBooleanProperties()
{
    static_assert(std::is_same_v<LayerPropertyAdapter::ValueType, bool>);
    static_assert(
        std::is_same_v<LayerPropertyAdapter::ConnectorType, MultinodePropertyBoolConnector<LayerPropertyAdapter>>);
    static_assert(!LayerPropertyAdapter::forceIgnoreByDefault);

    LayerFixture fixture;
    const KisBaseNode::Property item = firstBooleanProperty(fixture.first);
    QVERIFY(!item.name.isEmpty());
    LayerPropertyAdapter adapter(item.name);
    QCOMPARE(adapter.name(), item.name);
    QCOMPARE(adapter.propForNode(fixture.first), item.state.toBool());

    adapter.setPropForNode(fixture.first, !item.state.toBool(), 4);
    QCOMPARE(adapter.propForNode(fixture.first), !item.state.toBool());
    adapter.setPropForNode(fixture.first, item.state.toBool(), -1);
    QCOMPARE(adapter.propForNode(fixture.first), item.state.toBool());

    LayerPropertyAdapter missing(QStringLiteral("missing property"));
    QVERIFY(!missing.propForNode(fixture.first));
    const KisBaseNode::PropertyList before = constState(fixture.first).properties;
    missing.setPropForNode(fixture.first, true, 0);
    QVERIFY(constState(fixture.first).properties == before);

    state(fixture.second).properties.prepend(makeProperty(QStringLiteral("solo"), QStringLiteral("Solo"), true));
    const KisBaseNode::PropertyList adapters = LayerPropertyAdapter::adaptersList(fixture.nodes());
    QVERIFY(!adapters.isEmpty());
    QSet<QString> ids;
    int soloIndex = -1;
    int lockedIndex = -1;
    int index = 0;
    for (const KisBaseNode::Property &property : adapters) {
        QVERIFY(isBoolean(property.state));
        QVERIFY(!ids.contains(property.id));
        ids.insert(property.id);
        if (property.id == QStringLiteral("solo")) {
            soloIndex = index;
        } else if (property.id == QStringLiteral("locked")) {
            lockedIndex = index;
        }
        ++index;
    }
    QVERIFY(soloIndex >= 0);
    QVERIFY(lockedIndex >= 0);
    QVERIFY(soloIndex < lockedIndex);

    KisBaseNode::Property sameName = item;
    sameName.state = !item.state.toBool();
    QCOMPARE(qHash(item, 31), qHash(sameName, 31));
}

void KisMultinodePropertyTest::channelFlagAdaptersRequireCompatibleLayers()
{
    static_assert(std::is_same_v<ChannelFlagAdapter::ValueType, bool>);
    static_assert(
        std::is_same_v<ChannelFlagAdapter::ConnectorType, MultinodePropertyBoolConnector<ChannelFlagAdapter>>);
    static_assert(std::is_same_v<ChannelFlagAdapter::PropertyList, QList<ChannelFlagAdapter::Property>>);
    static_assert(!ChannelFlagAdapter::forceIgnoreByDefault);

    LayerFixture fixture;
    const ChannelFlagAdapter::PropertyList properties = ChannelFlagAdapter::adaptersList(fixture.nodes());
    QCOMPARE(properties.size(), 4);
    QCOMPARE(properties.first().name, QStringLiteral("Red"));
    QCOMPARE(properties.first().channelIndex, 0);

    ChannelFlagAdapter adapter(properties.first());
    QCOMPARE(adapter.name(), properties.first().name);
    QVERIFY(adapter.propForNode(fixture.first));
    adapter.setPropForNode(fixture.first, false, 8);
    QVERIFY(!adapter.propForNode(fixture.first));
    adapter.setPropForNode(fixture.first, true, -1);
    QVERIFY(adapter.propForNode(fixture.first));

    state(fixture.second).layer = false;
    const KisNodeList filtered = ChannelFlagAdapter::filterNodes(fixture.nodes());
    QCOMPARE(filtered, KisNodeList({fixture.first, fixture.third}));

    state(fixture.second).layer = true;
    state(fixture.second).colorSpace = reinterpret_cast<const KoColorSpace *>(quintptr(2));
    QVERIFY(ChannelFlagAdapter::adaptersList(fixture.nodes()).isEmpty());
}

void KisMultinodePropertyTest::multinodePropertyTracksUniformIgnoredAndRereadStates()
{
    static_assert(std::is_same_v<KisMultinodeCompositeOpProperty, KisMultinodeProperty<CompositeOpAdapter>>);
    static_assert(std::is_same_v<KisMultinodeOpacityProperty, KisMultinodeProperty<OpacityAdapter>>);
    static_assert(std::is_same_v<KisMultinodeNameProperty, KisMultinodeProperty<NameAdapter>>);
    static_assert(std::is_same_v<KisMultinodeColorLabelProperty, KisMultinodeProperty<ColorLabelAdapter>>);
    static_assert(std::is_same_v<KisMultinodeCompositeOpProperty::ValueType, QString>);
    static_assert(std::is_same_v<KisMultinodeCompositeOpProperty::ConnectorType, MultinodePropertyBaseConnector>);

    LayerFixture fixture;
    KisMultinodeCompositeOpProperty composite(fixture.nodes());
    QVERIFY(!composite.isIgnored());
    QVERIFY(!composite.savedValuesDiffer());
    QVERIFY(!composite.haveTheOnlyNode());
    QCOMPARE(composite.value(), normalCompositeOp);

    composite.connectValueChangedSignal(this, SLOT(recordValueChanged()));
    QCOMPARE(m_valueChangedCount, 1);
    composite.setValue(darkenCompositeOp);
    QCOMPARE(m_valueChangedCount, 2);
    composite.setValue(darkenCompositeOp);
    QCOMPARE(m_valueChangedCount, 2);
    for (const KisNodeSP &node : fixture.nodes()) {
        QCOMPARE(constState(node).compositeOp, darkenCompositeOp);
    }

    composite.setIgnored(true);
    QVERIFY(composite.isIgnored());
    QCOMPARE(composite.value(), normalCompositeOp);
    for (const KisNodeSP &node : fixture.nodes()) {
        QCOMPARE(constState(node).compositeOp, normalCompositeOp);
    }
    composite.setIgnored(false);
    QVERIFY(!composite.isIgnored());

    KisNodeList oneNode{fixture.first};
    KisMultinodeCompositeOpProperty single(oneNode);
    QVERIFY(single.haveTheOnlyNode());
    state(fixture.first).compositeOp = normalCompositeOp;
    single.rereadCurrentValue();
    QCOMPARE(single.value(), normalCompositeOp);

    KisMultinodeNameProperty names(fixture.nodes());
    QVERIFY(names.isIgnored());
    names.setIgnored(false);
    names.setValue(QStringLiteral("Renamed 99"));
    QCOMPARE(constState(fixture.first).name, QStringLiteral("Renamed 0"));
    QCOMPARE(constState(fixture.second).name, QStringLiteral("Renamed 1"));
    QCOMPARE(constState(fixture.third).name, QStringLiteral("Renamed 2"));

    KisMultinodeOpacityProperty opacity(fixture.nodes());
    opacity.setValue(25);
    QCOMPARE(constState(fixture.first).opacity, quint8(64));

    KisMultinodeColorLabelProperty colorLabel(fixture.nodes());
    colorLabel.setValue(4);
    QCOMPARE(constState(fixture.second).colorLabel, 4);

    KisMultinodePropertyInterfaceSP interface(new KisMultinodeCompositeOpProperty(fixture.nodes()));
    QVERIFY(!interface.isNull());
    QVERIFY(!interface->isIgnored());
    QVERIFY(!interface->savedValuesDiffer());
    QVERIFY(!interface->haveTheOnlyNode());
    m_valueChangedCount = 0;
    interface->connectValueChangedSignal(this, SLOT(recordValueChanged()));
    QCOMPARE(m_valueChangedCount, 1);
    QCheckBox interfaceIgnoreBox;
    interface->connectIgnoreCheckBox(&interfaceIgnoreBox);
    QWidget interfaceEditor;
    interface->connectAutoEnableWidget(&interfaceEditor);
    interface->setIgnored(true);
    QVERIFY(interface->isIgnored());
    interface->setIgnored(false);
    interface->rereadCurrentValue();
    QScopedPointer<KUndo2Command> interfaceCommand(interface->createPostExecutionUndoCommand());
    QVERIFY(interfaceCommand);
}

void KisMultinodePropertyTest::connectorWidgetsAndSignalsFollowIgnoreState()
{
    static_assert(std::is_base_of_v<QObject, MultinodePropertyConnectorInterface>);
    static_assert(std::is_base_of_v<MultinodePropertyConnectorInterface, MultinodePropertyBaseConnector>);

    FakeProperty property;
    MultinodePropertyBaseConnector connector(&property);
    QSignalSpy spy(&connector, &MultinodePropertyConnectorInterface::sigValueChanged);
    connector.connectValueChangedSignal(this, SLOT(recordValueChanged()));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(m_valueChangedCount, 1);
    connector.notifyValueChanged();
    QCOMPARE(spy.size(), 2);
    QCOMPARE(m_valueChangedCount, 2);

    QCheckBox ignoreBox;
    connector.connectIgnoreCheckBox(&ignoreBox);
    QVERIFY(ignoreBox.isEnabled());
    QVERIFY(!ignoreBox.isChecked());
    ignoreBox.setChecked(true);
    QVERIFY(!property.ignored);
    QCOMPARE(property.setIgnoredCount, 1);

    property.ignored = true;
    connector.notifyIgnoreChanged();
    QVERIFY(!ignoreBox.isChecked());

    QWidget editor;
    connector.MultinodePropertyConnectorInterface::connectAutoEnableWidget(&editor);
    QVERIFY(property.ignored);
    connector.connectAutoEnableWidget(&editor);
    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(1.0, 1.0),
                      QPointF(1.0, 1.0),
                      QPointF(1.0, 1.0),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(&editor, &press);
    QVERIFY(!property.ignored);
    QVERIFY(ignoreBox.isChecked());

    FakeProperty uniformProperty;
    uniformProperty.ignored = false;
    uniformProperty.valuesDiffer = false;
    MultinodePropertyBaseConnector uniformConnector(&uniformProperty);
    QCheckBox uniformBox;
    uniformConnector.connectIgnoreCheckBox(&uniformBox);
    QVERIFY(!uniformBox.isEnabled());
    QVERIFY(uniformBox.isChecked());

    FakeProperty singleProperty;
    singleProperty.ignored = false;
    singleProperty.valuesDiffer = false;
    singleProperty.onlyNode = true;
    MultinodePropertyBaseConnector singleConnector(&singleProperty);
    QCheckBox singleBox;
    singleBox.show();
    singleConnector.connectIgnoreCheckBox(&singleBox);
    QVERIFY(singleBox.isHidden());
}

void KisMultinodePropertyTest::boolConnectorMapsTriStateToProperty()
{
    LayerFixture fixture;
    state(fixture.second).properties[0].state = false;
    const KisBaseNode::Property visibleProperty = firstBooleanProperty(fixture.first);
    LayerPropertyAdapter adapter(visibleProperty.name);
    KisNodeList nodes{fixture.first, fixture.second};
    KisMultinodeProperty<LayerPropertyAdapter> property(nodes, adapter);
    QVERIFY(property.isIgnored());

    MultinodePropertyBoolConnector<LayerPropertyAdapter> connector(&property);
    QCheckBox box;
    connector.connectIgnoreCheckBox(&box);
    QVERIFY(box.isTristate());
    connector.notifyValueChanged();
    QCOMPARE(box.checkState(), Qt::PartiallyChecked);

    box.setCheckState(Qt::Checked);
    QVERIFY(!property.isIgnored());
    QVERIFY(property.value());
    QVERIFY(adapter.propForNode(fixture.first));
    QVERIFY(adapter.propForNode(fixture.second));

    connector.notifyIgnoreChanged();
    QCOMPARE(box.checkState(), Qt::Checked);
    box.setCheckState(Qt::Unchecked);
    QVERIFY(!property.value());
    QVERIFY(!adapter.propForNode(fixture.first));
    QVERIFY(!adapter.propForNode(fixture.second));
}

void KisMultinodePropertyTest::undoCommandsRestoreAndReapplyPerNodeValues()
{
    static_assert(std::is_same_v<MultinodePropertyUndoCommand<OpacityAdapter>::ValueType, int>);
    static_assert(std::is_base_of_v<KUndo2Command, MultinodePropertyUndoCommand<OpacityAdapter>>);

    LayerFixture fixture;
    state(fixture.first).opacity = 51;
    state(fixture.second).opacity = 102;
    KisNodeList nodes{fixture.first, fixture.second};
    MultinodePropertyUndoCommand<OpacityAdapter> directCommand(OpacityAdapter(), nodes, QList<int>({20, 40}), 60);
    directCommand.redo();
    QCOMPARE(OpacityAdapter::propForNode(fixture.first), 60);
    QCOMPARE(OpacityAdapter::propForNode(fixture.second), 60);
    directCommand.undo();
    QCOMPARE(OpacityAdapter::propForNode(fixture.first), 20);
    QCOMPARE(OpacityAdapter::propForNode(fixture.second), 40);

    state(fixture.first).compositeOp = darkenCompositeOp;
    state(fixture.second).compositeOp = normalCompositeOp;
    KisMultinodeCompositeOpProperty property(nodes);
    QVERIFY(property.savedValuesDiffer());
    QVERIFY(property.isIgnored());
    property.setIgnored(false);
    QScopedPointer<KUndo2Command> command(property.createPostExecutionUndoCommand());
    command->undo();
    QCOMPARE(constState(fixture.first).compositeOp, darkenCompositeOp);
    QCOMPARE(constState(fixture.second).compositeOp, normalCompositeOp);
    command->redo();
    QCOMPARE(constState(fixture.first).compositeOp, darkenCompositeOp);
    QCOMPARE(constState(fixture.second).compositeOp, darkenCompositeOp);
}

void KisMultinodePropertyTest::recordValueChanged()
{
    ++m_valueChangedCount;
}

QTEST_MAIN(KisMultinodePropertyTest)

#include "kis_multinode_property_test.moc"
