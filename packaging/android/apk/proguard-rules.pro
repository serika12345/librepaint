-dontobfuscate

-assumenosideeffects class android.util.Log {
    public static int v(...);
    public static int d(...);
}

-keep class org.qtproject.qt.android.** { *; }
-keep class org.krita.android.** { *; }
-keep class org.libsdl.app.** { *; }
