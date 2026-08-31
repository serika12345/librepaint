/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISIMAGEANIMSETTINGCOMMANDANIMATIONACCESS_P_H
#define KISIMAGEANIMSETTINGCOMMANDANIMATIONACCESS_P_H

class KisImageAnimationInterface;
class KisTimeSpan;

int kisImageAnimSettingCommandFramerate(const KisImageAnimationInterface *interface);
KisTimeSpan kisImageAnimSettingCommandDocumentRange(const KisImageAnimationInterface *interface);
void kisImageAnimSettingCommandSetFramerate(KisImageAnimationInterface *interface, int fps);
void kisImageAnimSettingCommandSetDocumentRange(KisImageAnimationInterface *interface, const KisTimeSpan &range);

#endif
