/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_serializable_configuration.h"

#include <QDomDocument>
#include <QTest>

#include <memory>

namespace
{

class ConfigurationProbe final : public KisSerializableConfiguration
{
public:
    explicit ConfigurationProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ConfigurationProbe(const ConfigurationProbe &rhs)
        : KisSerializableConfiguration(rhs)
        , parsedTag(rhs.parsedTag)
        , parsedName(rhs.parsedName)
        , parsedText(rhs.parsedText)
        , outputName(rhs.outputName)
        , outputText(rhs.outputText)
    {
    }

    ~ConfigurationProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    using KisSerializableConfiguration::fromXML;
    using KisSerializableConfiguration::toXML;

    void fromXML(const QDomElement &element) override
    {
        ++parseCalls;
        parsedTag = element.tagName();
        parsedName = element.attribute(QStringLiteral("name"));
        parsedText = element.text();
    }

    void toXML(QDomDocument &document, QDomElement &parent) const override
    {
        ++serializeCalls;
        serializedParent = parent.tagName();
        receivedDocumentRoot = document.documentElement() == parent;

        QDomElement entry = document.createElement(QStringLiteral("entry"));
        entry.setAttribute(QStringLiteral("name"), outputName);
        entry.appendChild(document.createTextNode(outputText));
        parent.appendChild(entry);
    }

    int parseCalls{0};
    QString parsedTag;
    QString parsedName;
    QString parsedText;
    QString outputName{QStringLiteral("contract")};
    QString outputText{QStringLiteral("A&B")};
    mutable int serializeCalls{0};
    mutable QString serializedParent;
    mutable bool receivedDocumentRoot{false};

private:
    int *m_destructionCount{nullptr};
};

struct FactoryCalls {
    int createDefault{0};
    int create{0};
    int destruction{0};
    QString receivedTag;
    QString receivedName;
};

class ConfigurationFactoryProbe final : public KisSerializableConfigurationFactory
{
public:
    ConfigurationFactoryProbe(FactoryCalls *calls, int *configurationDestructionCount)
        : m_calls(calls)
        , m_configurationDestructionCount(configurationDestructionCount)
    {
    }

    ~ConfigurationFactoryProbe() override
    {
        ++m_calls->destruction;
    }

    KisSerializableConfigurationSP createDefault() override
    {
        ++m_calls->createDefault;
        return new ConfigurationProbe(m_configurationDestructionCount);
    }

    KisSerializableConfigurationSP create(const QDomElement &element) override
    {
        ++m_calls->create;
        m_calls->receivedTag = element.tagName();
        m_calls->receivedName = element.attribute(QStringLiteral("name"));

        ConfigurationProbe *configuration = new ConfigurationProbe(m_configurationDestructionCount);
        configuration->fromXML(element);
        return configuration;
    }

private:
    FactoryCalls *m_calls;
    int *m_configurationDestructionCount;
};

} // namespace

class KisSerializableConfigurationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesValidXmlAndRejectsInvalidXml();
    void versionFlagDoesNotAffectParsing();
    void serializesThroughParamsRoot();
    void copyAndSharedOwnershipAreIndependent();
    void factoryRoutesCreationAndSupportsBaseLifetime();
};

void KisSerializableConfigurationContractTest::parsesValidXmlAndRejectsInvalidXml()
{
    ConfigurationProbe probe;
    KisSerializableConfiguration &configuration = probe;

    QVERIFY(configuration.fromXML(QStringLiteral("<configuration name=\"alpha\"><value>42</value></configuration>"),
                                  false));
    QCOMPARE(probe.parseCalls, 1);
    QCOMPARE(probe.parsedTag, QStringLiteral("configuration"));
    QCOMPARE(probe.parsedName, QStringLiteral("alpha"));
    QCOMPARE(probe.parsedText, QStringLiteral("42"));

    QVERIFY(!configuration.fromXML(QStringLiteral("<configuration>"), false));
    QCOMPARE(probe.parseCalls, 1);
}

void KisSerializableConfigurationContractTest::versionFlagDoesNotAffectParsing()
{
    ConfigurationProbe probe;
    KisSerializableConfiguration &configuration = probe;
    const QString xml = QStringLiteral("<configuration name=\"same\">payload</configuration>");

    QVERIFY(configuration.fromXML(xml, false));
    QCOMPARE(probe.parsedName, QStringLiteral("same"));
    QCOMPARE(probe.parsedText, QStringLiteral("payload"));

    QVERIFY(configuration.fromXML(xml, true));
    QCOMPARE(probe.parseCalls, 2);
    QCOMPARE(probe.parsedName, QStringLiteral("same"));
    QCOMPARE(probe.parsedText, QStringLiteral("payload"));
}

void KisSerializableConfigurationContractTest::serializesThroughParamsRoot()
{
    ConfigurationProbe probe;
    const KisSerializableConfiguration &configuration = probe;

    const QString xml = configuration.toXML();

    QCOMPARE(probe.serializeCalls, 1);
    QCOMPARE(probe.serializedParent, QStringLiteral("params"));
    QVERIFY(probe.receivedDocumentRoot);

    QDomDocument document;
    QVERIFY(document.setContent(xml));
    const QDomElement root = document.documentElement();
    QCOMPARE(root.tagName(), QStringLiteral("params"));
    const QDomElement entry = root.firstChildElement(QStringLiteral("entry"));
    QCOMPARE(entry.attribute(QStringLiteral("name")), QStringLiteral("contract"));
    QCOMPARE(entry.text(), QStringLiteral("A&B"));
}

void KisSerializableConfigurationContractTest::copyAndSharedOwnershipAreIndependent()
{
    ConfigurationProbe original;
    QDomDocument originalDocument;
    QVERIFY(originalDocument.setContent(QStringLiteral("<configuration name=\"original\">first</configuration>")));
    original.fromXML(originalDocument.documentElement());
    ConfigurationProbe copy(original);

    QDomDocument changedDocument;
    QVERIFY(changedDocument.setContent(QStringLiteral("<configuration name=\"changed\">second</configuration>")));
    original.fromXML(changedDocument.documentElement());

    QCOMPARE(copy.parsedName, QStringLiteral("original"));
    QCOMPARE(copy.parsedText, QStringLiteral("first"));
    QCOMPARE(original.parsedName, QStringLiteral("changed"));
    QCOMPARE(original.parsedText, QStringLiteral("second"));

    int destructions = 0;
    {
        KisSerializableConfigurationSP owner = new ConfigurationProbe(&destructions);
        QCOMPARE(owner->refCount(), 1);
        {
            KisSerializableConfigurationSP alias = owner;
            QCOMPARE(owner->refCount(), 2);
            QCOMPARE(alias.data(), owner.data());
        }
        QCOMPARE(owner->refCount(), 1);
        QCOMPARE(destructions, 0);
    }
    QCOMPARE(destructions, 1);
}

void KisSerializableConfigurationContractTest::factoryRoutesCreationAndSupportsBaseLifetime()
{
    FactoryCalls calls;
    int configurationDestructions = 0;
    std::unique_ptr<KisSerializableConfigurationFactory> factory =
        std::make_unique<ConfigurationFactoryProbe>(&calls, &configurationDestructions);

    KisSerializableConfigurationSP defaultConfiguration = factory->createDefault();
    QVERIFY(!defaultConfiguration.isNull());
    QCOMPARE(calls.createDefault, 1);

    QDomDocument document;
    QVERIFY(document.setContent(QStringLiteral("<configuration name=\"factory\">created</configuration>")));
    KisSerializableConfigurationSP createdConfiguration = factory->create(document.documentElement());
    QVERIFY(!createdConfiguration.isNull());
    QCOMPARE(calls.create, 1);
    QCOMPARE(calls.receivedTag, QStringLiteral("configuration"));
    QCOMPARE(calls.receivedName, QStringLiteral("factory"));

    const ConfigurationProbe *createdProbe = dynamic_cast<const ConfigurationProbe *>(createdConfiguration.data());
    QVERIFY(createdProbe);
    QCOMPARE(createdProbe->parseCalls, 1);
    QCOMPARE(createdProbe->parsedText, QStringLiteral("created"));

    factory.reset();
    QCOMPARE(calls.destruction, 1);
    QCOMPARE(configurationDestructions, 0);

    defaultConfiguration.clear();
    createdConfiguration.clear();
    QCOMPARE(configurationDestructions, 2);
}

QTEST_GUILESS_MAIN(KisSerializableConfigurationContractTest)

#include "KisSerializableConfigurationContractTest.moc"
