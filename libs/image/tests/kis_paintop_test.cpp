/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_paintop_test.h"

#include <simpletest.h>
#include "KoID.h"
#include "brushengine/kis_no_size_paintop_settings.h"
#include "brushengine/kis_uniform_paintop_property.h"
#include "kis_paintop.h"
#include "kis_painter.h"
#include "kis_spacing_information.h"

class TestPaintOp : public KisPaintOp
{
public:

    TestPaintOp(KisPainter * gc)
            : KisPaintOp(gc) {
    }

protected:

    KisSpacingInformation paintAt(const KisPaintInformation&) override {
        return KisSpacingInformation(0.0);
    }

    KisSpacingInformation updateSpacingImpl(const KisPaintInformation&) const override
    {
        return KisSpacingInformation(0.0);
    }

};

void KisPaintopTest::testCreation()
{
    KisPainter p;
    TestPaintOp test(&p);
}

void KisPaintopTest::testUniformPropertyConfiguration()
{
    KisPaintOpSettingsSP settings = new KisNoSizePaintOpSettings(KisResourcesInterfaceSP());
    KisUniformPaintOpProperty property(KoID("test"), settings, nullptr);

    QCOMPARE(property.configuration(), settings.data());

    settings.clear();
    QVERIFY(property.configuration());

    KisUniformPaintOpProperty emptyProperty(KoID("empty"), KisPaintOpSettingsSP(), nullptr);
    QVERIFY(!emptyProperty.configuration());
}


SIMPLE_TEST_MAIN(KisPaintopTest)
