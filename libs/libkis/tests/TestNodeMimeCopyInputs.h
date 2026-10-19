/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef LIBREPAINT_TEST_NODE_MIME_COPY_INPUTS_H
#define LIBREPAINT_TEST_NODE_MIME_COPY_INPUTS_H

#include <kis_mimedata.h>
#include <kis_node.h>
#include <QSet>
#include <QTest>

namespace {
/** Fixes prepared UUID inputs at the existing MIME insertion boundary. */
class FixedMimeCopyInputs final : public KisMimeData::NodeInsertionInterface
{
public:
    explicit FixedMimeCopyInputs(KisMimeData::NodeInsertionInterface *destination) : m_destination(destination) {}
    void moveNodes(KisNodeList nodes, KisNodeSP parent, KisNodeSP above) override
    { m_destination->moveNodes(nodes, parent, above); }
    void addNodes(KisNodeList nodes, KisNodeSP parent, KisNodeSP above) override
    {
        QList<QPair<KisNodeSP, QByteArray>> pending;
        for (qsizetype i = 0; i < nodes.size(); ++i) pending.append({nodes[i], QByteArray::number(i)});
        const QUuid fixture(QStringLiteral("{a1000000-1122-3344-5566-778899aabbcc}"));
        QSet<QUuid> generated;
        for (qsizetype i = 0; i < pending.size(); ++i) {
            KisNodeSP node = pending[i].first;
            QVERIFY(!node->uuid().isNull());
            QVERIFY(!generated.contains(node->uuid()));
            generated.insert(node->uuid());
            node->setUuid(QUuid::createUuidV5(fixture, pending[i].second));
            qsizetype childIndex = 0;
            for (auto child = node->firstChild(); child; child = child->nextSibling())
                pending.append({child, pending[i].second + '/' + QByteArray::number(childIndex++)});
        }
        m_destination->addNodes(nodes, parent, above);
    }
private:
    KisMimeData::NodeInsertionInterface *m_destination;
};
}

#endif
