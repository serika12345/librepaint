#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write the Xcode project of the iPad probe: PencilProbe (an application that
logs the touches it receives and flashes its window for the clock calibration)
and PencilProbeUITests (the UI test bundle whose runner sends synthesized
Apple Pencil strokes to testmanagerd).

Usage: genproj.py PROJECT_DIR DEVELOPMENT_TEAM
  PROJECT_DIR receives PencilProbe.xcodeproj and must contain App/ and
  UITests/ (run-ipad.sh copies them from this directory). The team signs the
  two targets with automatic signing.
"""
import os
import sys, itertools
c = itertools.count(1)
def i(): return "A0%022X" % next(c)
ids = {k: i() for k in """proj main grpApp grpTests grpProducts appSwift appPlist testM appRef testRef
appBF testBF appTarget testTarget appSrcPhase appFwPhase appResPhase testSrcPhase testFwPhase testResPhase
appCfgList testCfgList projCfgList appDbg appRel testDbg testRel projDbg projRel dep proxy""".split()}
d = ids
team = sys.argv[2]
common = 'SDKROOT = iphoneos; IPHONEOS_DEPLOYMENT_TARGET = 17.0; TARGETED_DEVICE_FAMILY = 2; CODE_SIGN_STYLE = Automatic; DEVELOPMENT_TEAM = %s; SWIFT_VERSION = 5.0; CLANG_ENABLE_OBJC_ARC = YES; ONLY_ACTIVE_ARCH = YES; ARCHS = arm64; ENABLE_USER_SCRIPT_SANDBOXING = NO;' % team
def cfg(id, name, extra): return '%s = {isa = XCBuildConfiguration; buildSettings = {%s %s}; name = %s; };' % (id, common, extra, name)
appS = 'PRODUCT_NAME = PencilProbe; PRODUCT_BUNDLE_IDENTIFIER = local.librepaint.pencilprobe; INFOPLIST_FILE = App/Info.plist; GENERATE_INFOPLIST_FILE = NO;'
testS = 'PRODUCT_NAME = PencilProbeUITests; PRODUCT_BUNDLE_IDENTIFIER = local.librepaint.pencilprobe.uitests; GENERATE_INFOPLIST_FILE = YES; TEST_TARGET_NAME = PencilProbe;'
out = f"""// !$*UTF8*$!
{{ archiveVersion = 1; classes = {{}}; objectVersion = 56; objects = {{
{d['appSwift']} = {{isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = App.swift; sourceTree = "<group>"; }};
{d['appPlist']} = {{isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = "<group>"; }};
{d['testM']} = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.objc; path = PencilProbeTests.m; sourceTree = "<group>"; }};
{d['appRef']} = {{isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = PencilProbe.app; sourceTree = BUILT_PRODUCTS_DIR; }};
{d['testRef']} = {{isa = PBXFileReference; explicitFileType = wrapper.cfbundle; includeInIndex = 0; path = PencilProbeUITests.xctest; sourceTree = BUILT_PRODUCTS_DIR; }};
{d['appBF']} = {{isa = PBXBuildFile; fileRef = {d['appSwift']}; }};
{d['testBF']} = {{isa = PBXBuildFile; fileRef = {d['testM']}; }};
{d['main']} = {{isa = PBXGroup; children = ({d['grpApp']}, {d['grpTests']}, {d['grpProducts']}); sourceTree = "<group>"; }};
{d['grpApp']} = {{isa = PBXGroup; children = ({d['appSwift']}, {d['appPlist']}); path = App; sourceTree = "<group>"; }};
{d['grpTests']} = {{isa = PBXGroup; children = ({d['testM']}); path = UITests; sourceTree = "<group>"; }};
{d['grpProducts']} = {{isa = PBXGroup; children = ({d['appRef']}, {d['testRef']}); name = Products; sourceTree = "<group>"; }};
{d['appSrcPhase']} = {{isa = PBXSourcesBuildPhase; files = ({d['appBF']}); }};
{d['appFwPhase']} = {{isa = PBXFrameworksBuildPhase; files = (); }};
{d['appResPhase']} = {{isa = PBXResourcesBuildPhase; files = (); }};
{d['testSrcPhase']} = {{isa = PBXSourcesBuildPhase; files = ({d['testBF']}); }};
{d['testFwPhase']} = {{isa = PBXFrameworksBuildPhase; files = (); }};
{d['testResPhase']} = {{isa = PBXResourcesBuildPhase; files = (); }};
{d['proxy']} = {{isa = PBXContainerItemProxy; containerPortal = {d['proj']}; proxyType = 1; remoteGlobalIDString = {d['appTarget']}; remoteInfo = PencilProbe; }};
{d['dep']} = {{isa = PBXTargetDependency; target = {d['appTarget']}; targetProxy = {d['proxy']}; }};
{d['appTarget']} = {{isa = PBXNativeTarget; buildConfigurationList = {d['appCfgList']}; buildPhases = ({d['appSrcPhase']}, {d['appFwPhase']}, {d['appResPhase']}); dependencies = (); name = PencilProbe; productName = PencilProbe; productReference = {d['appRef']}; productType = "com.apple.product-type.application"; }};
{d['testTarget']} = {{isa = PBXNativeTarget; buildConfigurationList = {d['testCfgList']}; buildPhases = ({d['testSrcPhase']}, {d['testFwPhase']}, {d['testResPhase']}); dependencies = ({d['dep']}); name = PencilProbeUITests; productName = PencilProbeUITests; productReference = {d['testRef']}; productType = "com.apple.product-type.bundle.ui-testing"; }};
{d['proj']} = {{isa = PBXProject; attributes = {{LastUpgradeCheck = 2600; TargetAttributes = {{ {d['testTarget']} = {{TestTargetID = {d['appTarget']}; }}; }}; }}; buildConfigurationList = {d['projCfgList']}; compatibilityVersion = "Xcode 14.0"; developmentRegion = en; knownRegions = (en, Base); mainGroup = {d['main']}; productRefGroup = {d['grpProducts']}; projectDirPath = ""; projectRoot = ""; targets = ({d['appTarget']}, {d['testTarget']}); }};
{cfg(d['appDbg'], 'Debug', appS)}
{cfg(d['appRel'], 'Release', appS)}
{cfg(d['testDbg'], 'Debug', testS)}
{cfg(d['testRel'], 'Release', testS)}
{cfg(d['projDbg'], 'Debug', 'DEBUG_INFORMATION_FORMAT = dwarf;')}
{cfg(d['projRel'], 'Release', '')}
{d['appCfgList']} = {{isa = XCConfigurationList; buildConfigurations = ({d['appDbg']}, {d['appRel']}); defaultConfigurationName = Debug; }};
{d['testCfgList']} = {{isa = XCConfigurationList; buildConfigurations = ({d['testDbg']}, {d['testRel']}); defaultConfigurationName = Debug; }};
{d['projCfgList']} = {{isa = XCConfigurationList; buildConfigurations = ({d['projDbg']}, {d['projRel']}); defaultConfigurationName = Debug; }};
}}; rootObject = {d['proj']}; }}
"""
project = os.path.join(sys.argv[1], "PencilProbe.xcodeproj")
os.makedirs(os.path.join(project, "xcshareddata", "xcschemes"), exist_ok=True)
open(os.path.join(project, "project.pbxproj"), "w").write(out)


def ref(target, name, product):
    return ('<BuildableReference BuildableIdentifier = "primary" BlueprintIdentifier = "%s" BuildableName = "%s" '
            'BlueprintName = "%s" ReferencedContainer = "container:PencilProbe.xcodeproj"/>' % (target, product, name))


scheme = f"""<?xml version="1.0" encoding="UTF-8"?>
<Scheme LastUpgradeVersion = "2600" version = "1.7">
  <BuildAction parallelizeBuildables = "YES" buildImplicitDependencies = "YES">
    <BuildActionEntries>
      <BuildActionEntry buildForTesting = "YES" buildForRunning = "YES" buildForProfiling = "NO" buildForArchiving = "NO" buildForAnalyzing = "NO">
        {ref(d['appTarget'], 'PencilProbe', 'PencilProbe.app')}
      </BuildActionEntry>
    </BuildActionEntries>
  </BuildAction>
  <TestAction buildConfiguration = "Debug" selectedDebuggerIdentifier = "" selectedLauncherIdentifier = "Xcode.IDEFoundation.Launcher.PosixSpawn" shouldUseLaunchSchemeArgsEnv = "YES">
    <Testables>
      <TestableReference skipped = "NO">
        {ref(d['testTarget'], 'PencilProbeUITests', 'PencilProbeUITests.xctest')}
      </TestableReference>
    </Testables>
  </TestAction>
  <LaunchAction buildConfiguration = "Debug" selectedDebuggerIdentifier = "" selectedLauncherIdentifier = "Xcode.IDEFoundation.Launcher.PosixSpawn" launchStyle = "0" useCustomWorkingDirectory = "NO" ignoresPersistentStateOnLaunch = "NO" debugDocumentVersioning = "YES" allowLocationSimulation = "YES">
    <BuildableProductRunnable runnableDebuggingMode = "0">
      {ref(d['appTarget'], 'PencilProbe', 'PencilProbe.app')}
    </BuildableProductRunnable>
  </LaunchAction>
</Scheme>
"""
open(os.path.join(project, "xcshareddata", "xcschemes", "PencilProbe.xcscheme"), "w").write(scheme)
