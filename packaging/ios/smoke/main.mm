#import <UIKit/UIKit.h>

@interface KritaSmokeAppDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow *window;
@end

@implementation KritaSmokeAppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    (void)application;
    (void)launchOptions;

    CGRect frame = UIScreen.mainScreen.bounds;
    UIViewController *controller = [[UIViewController alloc] init];
    controller.view.backgroundColor = UIColor.systemBackgroundColor;

    UILabel *label = [[UILabel alloc] initWithFrame:frame];
    label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    label.text = @"LibrePaint iPadOS build environment is working";
    label.textAlignment = NSTextAlignmentCenter;
    label.numberOfLines = 0;
    [controller.view addSubview:label];

    self.window = [[UIWindow alloc] initWithFrame:frame];
    self.window.rootViewController = controller;
    [self.window makeKeyAndVisible];
    return YES;
}

@end

int main(int argc, char *argv[])
{
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(KritaSmokeAppDelegate.class));
    }
}
