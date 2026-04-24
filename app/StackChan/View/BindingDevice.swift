/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

import SwiftUI
import NetworkExtension
import CoreLocation
import NetworkExtension
import CoreBluetooth

struct BindingDevice : View {
    
    enum BindingDevicePageType: Hashable {
        case scanningEquipment
    }
    
    @State private var path: [BindingDevicePageType] = []
    
    var body: some View {
        NavigationStack(path: $path) {
            VStack{
                Spacer()
                VStack(alignment: .leading, spacing: 16) {
                    
                    HStack {
                        Spacer()
                        Image("lateral_image")
                            .resizable()
                            .frame(maxWidth: 250,maxHeight: 250)
                        Spacer()
                    }
                    
                    Text("先准备好你的 StackChan")
                        .font(.title2)
                        .bold()
                    
                    VStack(alignment: .leading, spacing: 12) {
                        HStack(alignment: .top) {
                            Image(systemName: "1.circle.fill")
                                .foregroundColor(.accentColor)
                            Text("打开你的 StackChan 设备")
                        }
                        HStack(alignment: .top) {
                            Image(systemName: "2.circle.fill")
                                .foregroundColor(.accentColor)
                            Text("开机后切到机器人上的“设置”页并进入配网页面，屏幕上会显示二维码")
                        }
                        HStack(alignment: .top) {
                            Image(systemName: "3.circle.fill")
                                .foregroundColor(.accentColor)
                            Text("对准二维码扫描，即可绑定设备")
                        }
                    }
                    .font(.body)
                }
                .padding()
                Spacer()
                NavigationLink(value: BindingDevicePageType.scanningEquipment) {
                    Text("下一步")
                        .frame(maxWidth: .infinity)
                }
                .padding()
                .controlSize(.large)
                .buttonStyle(.borderedProminent)
            }
            .navigationTitle("绑定设备")
            .navigationDestination(for: BindingDevicePageType.self) { PageType in
                switch PageType {
                case .scanningEquipment:
                    ScanningEquipment()
                }
            }
        }
    }
}

struct ScanningEquipment : View {
    
    enum PairingStatus {
        case ScanCode
        case ConnectBlue
        case InputWiFi
        case DistributionNetwork
        case ChangeTheName
        case Empty
    }
    
    enum Field {
        case Name
        case Password
        case StackChanName
    }
    
    @EnvironmentObject var appState: AppState
    
    @State var pairingStatus: PairingStatus = .ScanCode
    
    @State var wifiName: String = ""
    @State var wifiPassword: String = ""
    @State var stackChanName: String = ""
    
    @State private var locationManager = CLLocationManager()
    @State private var locationDelegate = LocationDelegate()
    
    @FocusState private var focusedField: Field?
    
    var body: some View {
        Group {
            switch pairingStatus {
            case .ScanCode:
                GeometryReader { geometry in
                    ScanView { result in
                        switch result {
                        case .success(let data):
                            readCodeString(value: data)
                            break
                        case .failure(_):
                            break
                        }
                    }
                    .clipShape(
                        RoundedRectangle(
                            cornerRadius: min(geometry.size.width, geometry.size.height) * 0.1,
                            style: .continuous
                        )
                    )
                }
                .padding()
                .navigationTitle("扫描设备二维码")
            case .ConnectBlue:
                VStack {
                    ProgressView()
                        .progressViewStyle(.circular)
                    Text("正在连接蓝牙")
                        .font(.title3)
                        .multilineTextAlignment(.center)
                }
                .frame(maxWidth: .infinity,maxHeight: .infinity, alignment: .center)
                .navigationTitle("配对设备")
            case .InputWiFi:
                VStack {
                    List {
                        Section(header: Text("Wi-Fi 名称")) {
                            TextField("请输入 Wi-Fi 名称", text:$wifiName)
                                .focused($focusedField, equals: .Name)
                                .submitLabel(.next)
                                .onSubmit {
                                    focusedField = .Password
                                }
                        }
                        Section(header: Text("Wi-Fi 密码")) {
                            TextField("请输入 Wi-Fi 密码", text:$wifiPassword)
                                .focused($focusedField, equals: .Password)
                                .submitLabel(.done)
                                .onSubmit {
                                    confirmWifi()
                                }
                        }
                    }
                    .listStyle(.insetGrouped)
                    
                    Spacer()
                    
                    Button {
                        focusedField = nil
                        confirmWifi()
                    } label: {
                        Text("确定")
                            .frame(maxWidth: .infinity)
                    }
                    .padding()
                    .controlSize(.large)
                    .buttonStyle(.borderedProminent)
                }
                .background(Color(UIColor.systemGroupedBackground))
                .navigationTitle("输入 Wi-Fi 信息")
            case .DistributionNetwork:
                VStack {
                    ProgressView()
                        .progressViewStyle(.circular)
                    Text("正在为设备配置网络")
                        .font(.title3)
                        .multilineTextAlignment(.center)
                }
                .frame(maxWidth: .infinity,maxHeight: .infinity, alignment: .center)
                .navigationTitle("请稍候")
            case .ChangeTheName:
                VStack {
                    List {
                        Section(header: Text("设备名称")) {
                            TextField("请输入 StackChan 名称", text:$stackChanName)
                                .focused($focusedField, equals: .StackChanName)
                                .submitLabel(.done)
                                .onSubmit {
                                    updataName()
                                }
                        }
                    }
                    .listStyle(.insetGrouped)
                    
                    Spacer()
                    
                    Button {
                        focusedField = nil
                        updataName()
                    } label: {
                        Text("确定")
                            .frame(maxWidth: .infinity)
                    }
                    .padding()
                    .controlSize(.large)
                    .buttonStyle(.borderedProminent)
                }
                .frame(maxWidth: .infinity,maxHeight: .infinity, alignment: .center)
                .navigationTitle("给我起个名字")
            default:
                EmptyView()
            }
        }
        .alert(appState.alertTitle, isPresented: $appState.showAlert){
            Button {
                appState.alertAction?()
            } label: {
                Text("确定")
            }
        }
        .task {
        }
    }
    
    func updataName() {
        
    }
    
    func readCodeString(value: String) {
        if let data = value.data(using: .utf8), let json = try? JSONSerialization.jsonObject(with: data) as? [String:Any], let mac = json["mac"] as? String {
            let extracted = mac
            let cleanedMac = extracted.uppercased().replacingOccurrences(
                of: "[^A-F0-9]",
                with: "",
                options: .regularExpression
            )
            appState.deviceMac = cleanedMac
            appState.showBindingDevice = false
            appState.connectWebSocket()
            appState.openBlufi()
        }
    }
    
    private func getBlueAndWifiInfo() {
        NEHotspotNetwork.fetchCurrent { network in
            if let network = network {
                wifiName = network.ssid
                focusedField = .Password
            }
        }
        BlufiUtil.shared.startScan()
    }
    
    private func getPermission() {
        if #available(iOS 14.0, *) {
            switch locationManager.authorizationStatus {
            case .authorizedWhenInUse, .authorizedAlways:
                getBlueAndWifiInfo()
                break
            case .denied, .restricted:
                break
            case .notDetermined:
                locationManager.requestWhenInUseAuthorization()
                break
            default:
                break
            }
        } else {
            locationManager.requestWhenInUseAuthorization()
        }
    }
    
    private func confirmWifi() {
        
        if !BlufiUtil.shared.blueSwitch {
            appState.alertTitle = "请先打开蓝牙"
            appState.showAlert = true
            return
        }
        
        if wifiName.isEmpty || wifiPassword.isEmpty {
            appState.alertTitle = "请输入完整的 Wi-Fi 名称和密码"
            appState.showAlert = true
            return
        }
        
        withAnimation{
            pairingStatus = .DistributionNetwork
        }
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
            withAnimation{
                pairingStatus = .ChangeTheName
            }
        }
    }
    
    private func hideKeyboard() {
        UIApplication.shared.sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
    }
}


class LocationDelegate: NSObject,CLLocationManagerDelegate {
    var onAuthorized: (() -> Void)?
    
    func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
        if manager.authorizationStatus == .authorizedWhenInUse || manager.authorizationStatus == .authorizedAlways {
            onAuthorized?()
        }
    }
}
