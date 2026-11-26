// SPDX-License-Identifier: GPL-3.0-or-later
//
// PencilProbe: the iPad side of the pen latency measurement (run-ipad.sh).
// The canvas shows the last touch it received (kind, position, force, angles
// and the system time) in its status label, which testCalibrate reads, and
// writes each stroke's touches to Documents/touches-<time>.txt. With the
// launch argument -flash the window flashes for the clock calibration.
import UIKit

// Logs every touch the canvas receives: kind, force, angles and timestamps,
// so a replayed event stream can be told apart from finger input.
final class Canvas: UIView {
    var path = UIBezierPath()
    var lines: [String] = []
    let label = UILabel()
    override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .white
        label.numberOfLines = 0
        label.font = .monospacedSystemFont(ofSize: 14, weight: .regular)
        label.accessibilityIdentifier = "status"
        label.text = "ready"
        addSubview(label)
        isAccessibilityElement = false
        accessibilityIdentifier = "canvas"
        isMultipleTouchEnabled = false
        addGestureRecognizer(UIHoverGestureRecognizer(target: self, action: #selector(hover(_:))))
    }
    required init?(coder: NSCoder) { fatalError() }
    override func layoutSubviews() { label.frame = CGRect(x: 20, y: 40, width: bounds.width - 40, height: 160) }
    func log(_ phase: String, _ t: UITouch, _ event: UIEvent?) {
        let p = t.location(in: self)
        let kind = t.type == .pencil ? "pencil" : t.type == .direct ? "direct" : "other\(t.type.rawValue)"
        let coalesced = event?.coalescedTouches(for: t)?.count ?? 0
        let line = String(format: "%@ %@ x=%.1f y=%.1f force=%.3f max=%.3f alt=%.3f azi=%.3f ts=%.4f coalesced=%d",
                          phase, kind, p.x, p.y, t.force, t.maximumPossibleForce, t.altitudeAngle,
                          t.azimuthAngle(in: self), t.timestamp, coalesced)
        lines.append(line)
        NSLog("PencilProbe %@", line)
        label.text = "\(lines.count) events\n" + line
        label.accessibilityValue = line
    }
    @objc func hover(_ g: UIHoverGestureRecognizer) {
        let p = g.location(in: self)
        let line = String(format: "hover%ld x=%.1f y=%.1f z=%.3f", g.state.rawValue, p.x, p.y, g.zOffset)
        lines.append(line); NSLog("PencilProbe %@", line)
    }
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        lines.removeAll()
        for t in touches { log("began", t, event); path = UIBezierPath(); path.move(to: t.location(in: self)) }
    }
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for t in touches { log("moved", t, event); path.addLine(to: t.location(in: self)) }
        setNeedsDisplay()
    }
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for t in touches { log("ended", t, event) }
        let url = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].appendingPathComponent("touches-\(Int(Date().timeIntervalSince1970 * 1000)).txt")
        try? lines.joined(separator: "\n").write(to: url, atomically: true, encoding: .utf8)
        label.text = "done \(lines.count) events\n" + (lines.last ?? "")
    }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) { for t in touches { log("cancelled", t, event) } }
    override func draw(_ rect: CGRect) { UIColor.black.setStroke(); path.lineWidth = 4; path.stroke() }
}


// Flash mode (launch argument -flash): alternates the whole window between
// black and white every 13 frames of the 120 Hz display link and logs the
// target display time of each change, so a capture on another machine can map
// its clock to this one.
final class Flasher: NSObject {
    let view: UIView
    var link: CADisplayLink?
    var frame = 0
    var lines: [String] = []
    let period: Int
    let count: Int
    init(view: UIView, period: Int, count: Int) { self.view = view; self.period = period; self.count = count; super.init() }
    func start() {
        NSLog("PencilProbe flash start")
        link = CADisplayLink(target: self, selector: #selector(tick(_:)))
        link?.preferredFrameRateRange = CAFrameRateRange(minimum: 120, maximum: 120, preferred: 120)
        link?.add(to: .main, forMode: .common)
    }
    @objc func tick(_ l: CADisplayLink) {
        frame += 1
        if frame % period != 0 { return }
        let white = (frame / period) % 2 == 0
        CATransaction.begin(); CATransaction.setDisableActions(true)
        view.backgroundColor = white ? .white : .black
        CATransaction.commit()
        lines.append(String(format: "%@,%.6f,%.6f", white ? "white" : "black", l.targetTimestamp, CACurrentMediaTime()))
        if lines.count >= count {
            l.invalidate()
            let url = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].appendingPathComponent("flash-\(Int(Date().timeIntervalSince1970 * 1000)).csv")
            try? (["color,target_s,callback_s"] + lines).joined(separator: "\n").write(to: url, atomically: true, encoding: .utf8)
            NSLog("PencilProbe flash done %@", url.path)
        }
    }
}

final class ViewController: UIViewController {
    var flasher: Flasher?
    override func loadView() { view = Canvas(frame: .zero) }
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        if ProcessInfo.processInfo.arguments.contains("-flash") {
            (view as? Canvas)?.label.isHidden = true
            let cover = UIView(frame: view.bounds)
            cover.autoresizingMask = [.flexibleWidth, .flexibleHeight]
            cover.backgroundColor = .gray
            view.addSubview(cover)
            NSLog("PencilProbe flash mode")
            // -flash-count N changes the number of colour changes (default 60).
            let arguments = ProcessInfo.processInfo.arguments
            let index = arguments.firstIndex(of: "-flash-count")
            let count = index.flatMap { arguments.indices.contains($0 + 1) ? Int(arguments[$0 + 1]) : nil } ?? 60
            // An odd period of 120 Hz frames makes the changes alternate
            // between frames the 60 Hz capture sees and frames it skips, so the
            // calibration can tell the on-time changes from the late ones.
            flasher = Flasher(view: cover, period: 13, count: count)
            DispatchQueue.main.asyncAfter(deadline: .now() + 2) { self.flasher?.start() }
        }
    }
}

@main
final class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?
    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        window = UIWindow(frame: UIScreen.main.bounds)
        window?.rootViewController = ViewController()
        window?.makeKeyAndVisible()
        return true
    }
}
