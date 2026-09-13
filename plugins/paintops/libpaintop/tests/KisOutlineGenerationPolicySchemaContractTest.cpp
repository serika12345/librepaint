/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_outline_generation_policy.h>

#include <QTest>

#include <type_traits>

namespace
{

class OutlinePolicyParent
{
public:
    explicit OutlinePolicyParent(KisResourcesInterfaceSP)
    {
    }

    virtual ~OutlinePolicyParent() = default;

    virtual void onPropertyChanged()
    {
    }
};

} // namespace

class KisOutlineGenerationPolicySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void outlineGenerationPolicySchemaRemainStable();
};

void KisOutlineGenerationPolicySchemaContractTest::outlineGenerationPolicySchemaRemainStable()
{
    using Policy = KisOutlineGenerationPolicy<OutlinePolicyParent>;

    static_assert(std::is_class_v<Policy>);
    static_assert(std::is_base_of_v<OutlinePolicyParent, Policy>);
    static_assert(std::is_constructible_v<Policy, KisCurrentOutlineFetcher::Options, KisResourcesInterfaceSP>);
    static_assert(std::is_destructible_v<Policy>);
    static_assert(
        std::is_same_v<decltype(&Policy::outlineFetcher), const KisCurrentOutlineFetcher *(Policy::*)() const>);
    static_assert(std::is_same_v<decltype(&Policy::onPropertyChanged), void (Policy::*)()>);
}

QTEST_GUILESS_MAIN(KisOutlineGenerationPolicySchemaContractTest)

#include "KisOutlineGenerationPolicySchemaContractTest.moc"
