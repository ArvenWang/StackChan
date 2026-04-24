/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

import SwiftUI
import CoreBluetooth

struct ContentView: View {
    
    @EnvironmentObject var appState: AppState
    
    var body: some View {
        TabView {
            StackChan()
                .tabItem {
                    Label("机器人", systemImage: "ipod")
                }
            Nearby()
                .tabItem {
                    Label("附近", systemImage: "sensor")
                }
            Moments()
                .tabItem {
                    Label("动态", systemImage: "person.3")
                }
            Settings()
                .tabItem {
                    Label("设置", systemImage: "gear")
                }
        }
        .task {
            appState.openBlufi()
            if appState.deviceMac != "" {
                appState.connectWebSocket()
            }
        }
        .sheet(isPresented: $appState.showBindingDevice) {
            BindingDevice()
                .interactiveDismissDisabled(appState.forcedDisplayBindingDevice)
        }
        .sheet(isPresented: $appState.showDeviceWifiSet) {
            SelectBlufiDevice()
                .presentationDetents([.medium])
                .interactiveDismissDisabled(true)
        }
        .alert("给你的 StackChan 起个名字吧", isPresented: $appState.showCjamgeNameAlert, actions: {
            TextField("请输入名称", text: $appState.newName)
            Button("取消", role: .cancel) {
                appState.showCjamgeNameAlert = false
            }
            Button("确定") {
                appState.showCjamgeNameAlert = false
                withAnimation {
                    appState.deviceInfo.name = appState.newName
                }
                appState.updateDeviceInfo()
            }
        })
        .alert("请先把 StackChan 切到“设置”页面，在机器人上进入配网页，再回到 App 的“设置”里选择“绑定设备”。", isPresented: $appState.showBindingDeviceAlert) {
            Button("确定") {
                appState.showBindingDeviceAlert = false
            }
        }
    }
}


struct ContentViewPreview : PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
