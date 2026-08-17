/*
 *  kis_resourceserver.cc - part of KImageShop
 *
 *  SPDX-FileCopyrightText: 1999 Matthias Elter <elter@kde.org>
 *  SPDX-FileCopyrightText: 2003 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2005 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResourceServerProvider.h"

#include <QGlobalStatic>

#include <KoResourceServer.h>

#include <kis_workspace_resource.h>
#include <KisWindowLayoutResource.h>
#include <KisSessionResource.h>

Q_GLOBAL_STATIC(KisResourceServerProvider, s_instance)

KisResourceServerProvider::KisResourceServerProvider()
{
    m_workspaceServer = new KoResourceServer<KisWorkspaceResource>(ResourceType::Workspaces);
    m_windowLayoutServer = new KoResourceServer<KisWindowLayoutResource>(ResourceType::WindowLayouts);
    m_sessionServer = new KoResourceServer<KisSessionResource>(ResourceType::Sessions);
}

KisResourceServerProvider::~KisResourceServerProvider()
{
    delete m_workspaceServer;
    delete m_sessionServer;
    delete m_windowLayoutServer;
}

KisResourceServerProvider* KisResourceServerProvider::instance()
{
    return s_instance;
}


KoResourceServer< KisWorkspaceResource >* KisResourceServerProvider::workspaceServer()
{
    return m_workspaceServer;
}

KoResourceServer< KisWindowLayoutResource >* KisResourceServerProvider::windowLayoutServer()
{
    return m_windowLayoutServer;
}

KoResourceServer< KisSessionResource >* KisResourceServerProvider::sessionServer()
{
    return m_sessionServer;
}
