/*
 * SPDX-FileCopyrightText: 2020 Saurabh Kumar <saurabhk660@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "StoryboardModel.h"

void StoryboardModel::slotCommentDataChanged()
{
    m_commentList = m_commentModel->m_commentList;

    if (m_commentList.isEmpty()) {
        return;
    }

    for (int row = 0; row < rowCount(); ++row) {
        const QModelIndex board = index(row, 0);
        Q_EMIT dataChanged(index(StoryboardItem::Comments, 0, board),
                           index(StoryboardItem::Comments + m_commentList.size() - 1, 0, board));
    }
}

void StoryboardModel::slotCommentRowInserted(const QModelIndex parent, int first, int last)
{
    Q_UNUSED(parent);
    const int numItems = rowCount();
    for (int row = 0; row < numItems; ++row) {
        insertRows(StoryboardItem::Comments + first,
                   last - first + 1,
                   index(row, 0));
    }
    m_commentList = m_commentModel->m_commentList;
}

void StoryboardModel::slotCommentRowRemoved(const QModelIndex parent, int first, int last)
{
    Q_UNUSED(parent);
    const int numItems = rowCount();
    for (int row = 0; row < numItems; ++row) {
        removeRows(StoryboardItem::Comments + first,
                   last - first + 1,
                   index(row, 0));
    }
    m_commentList = m_commentModel->m_commentList;
}

void StoryboardModel::slotCommentRowMoved(const QModelIndex &sourceParent,
                                          int start,
                                          int end,
                                          const QModelIndex &destinationParent,
                                          int destinationRow)
{
    Q_UNUSED(sourceParent);
    Q_UNUSED(destinationParent);
    const int numItems = rowCount();
    for (int row = 0; row < numItems; ++row) {
        const QModelIndex board = index(row, 0);
        moveRowsImpl(board,
                     start + StoryboardItem::Comments,
                     end - start + 1,
                     board,
                     destinationRow + StoryboardItem::Comments);
    }
    m_commentList = m_commentModel->m_commentList;
}
