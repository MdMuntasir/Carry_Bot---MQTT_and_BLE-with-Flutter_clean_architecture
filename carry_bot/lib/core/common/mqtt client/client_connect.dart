import 'dart:developer';
import 'dart:io';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:carry_bot/core/common/mqtt%20client/client_state.dart';
import 'package:carry_bot/core/connection%20state/data_state.dart';
import 'package:internet_connection_checker_plus/internet_connection_checker_plus.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import '../../network/connection_checker.dart';

class MQTTClient {
  MqttServerClient? client;
  ConnectionChecker connectionChecker =
      ConnectionCheckerImpl(InternetConnection());

  Future<DataState<MqttServerClient>> connectClient(
      String userName, String password, String clusterURL, String name) async {
    if (await connectionChecker.isConnected) {
      client = MqttServerClient.withPort(clusterURL, name, 8883);
      client?.secure = true;
      client?.securityContext = SecurityContext.defaultContext;
      client?.keepAlivePeriod = 20;

      try {
        await client?.connect(userName, password);
        return DataSuccess(client!);
      } on Exception catch (e) {
        DataFailed(e.toString());
      }
    }
    return DataFailed("No Internet Connection");
  }

  ClientState getClient() {
    final mqttStatus = client?.connectionStatus;
    if (mqttStatus?.state == MqttConnectionState.connected) {
      return ClientConnected(client!);
    }
    return ClientDisconnected(mqttStatus?.disconnectionOrigin.toString());
  }
}

class BLEService {
  List<ScanResult> scanResults = [];
  BluetoothDevice? connectedDevice;
  BluetoothCharacteristic? characteristic;
  Function(List<ScanResult>)? onScanUpdated;
  Function(String)? onMessageReceived;

  void startScanning() {
    scanResults.clear();
    FlutterBluePlus.startScan(timeout: Duration(seconds: 5));

    FlutterBluePlus.scanResults.listen((results) {
      scanResults = results;
      if (onScanUpdated != null) {
        onScanUpdated!(scanResults);
      }
    });
  }

  void stopScanning() {
    FlutterBluePlus.stopScan();
  }

  Future<bool> connectToDevice(BluetoothDevice device) async {
    await device.connect();

    if(device.isConnected) {
      List<BluetoothService> services = await device.discoverServices();
      for (var service in services) {
        for (var char in service.characteristics) {
          if (char.properties.write) { // ✅ Find first writable characteristic
            characteristic = char;
            log("✅ Found writable characteristic: ${char.uuid}");
            break;
          }
        }
      }
      return true;
    }

      return false;


    // List<BluetoothService> services = await device.discoverServices();
    // for (var service in services) {
    //   for (var characteristic in service.characteristics) {
    //     if (characteristic.uuid.toString() == "PIN_CHARACTERISTIC_UUID") {
    //       await characteristic.write(pin.codeUnits);
    //       log("🔑 PIN Sent: $pin");
    //     }
    //   }
    // }

    connectedDevice = device;
  }

  void enableNotifications() async {
    if (connectedDevice != null) {
      List<BluetoothService> services =
          await connectedDevice!.discoverServices();
      for (var service in services) {
        for (var char in service.characteristics) {
          if (char.properties.notify) {
            await char.setNotifyValue(true);
            char.lastValueStream.listen((value) {
              String message = String.fromCharCodes(value);
              log("📩 New message: $message");
              if (onMessageReceived != null) {
                onMessageReceived!(message); // 🔥 Auto-update UI
              }
            });
          }
        }
      }
    }
  }

  Future<void> sendData(String data) async {
    if (characteristic != null) {
      await characteristic!.write(data.codeUnits);
      print("📤 Sent to ESP32: $data");
    } else {
      print("⚠️ No connected device!");
    }
  }
}
