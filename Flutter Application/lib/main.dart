import 'package:capstone/device_model.dart';
import 'package:capstone/device_widget.dart';
import 'package:capstone/test_dialog.dart';
import 'package:flutter/material.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Home Device Manager',
      theme: ThemeData(
        brightness: Brightness.dark,
        primarySwatch: Colors.teal,
        visualDensity: VisualDensity.adaptivePlatformDensity,
      ),
      home: const HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final List<Device> _devices = [
    Device(name: 'lamp00001', type: 'lamp', isOn: true, voltage: 120.5, current: 0.5), // low power
    Device(name: 'tv00001', type: 'tv', isOn: false, voltage: 120.5, current: 1.2), // off
    Device(name: 'computer00001', type: 'computer', isOn: true, voltage: 120.5, current: 50), // high power
    Device(name: 'fan00001', type: 'fan', isOn: true, voltage: 120.5, current: 0.8), // low power
    Device(name: 'lamp00002', type: 'lamp', isOn: false, voltage: 120.5, current: 0.5), // off
  ];

  void _toggleDevice(int index) {
    setState(() {
      _devices[index].isOn = !_devices[index].isOn;
    });
  }

  void _increaseVoltage(int index) {
    setState(() {
      _devices[index].voltage += 10;
    });
  }

  void _increaseCurrent(int index) {
    setState(() {
      _devices[index].current += 10;
    });
  }

  void _showTestDialog() {
    showDialog(
      context: context,
      builder: (context) {
        return TestDialog(
          devices: _devices,
          onVoltageUp: _increaseVoltage,
          onCurrentUp: _increaseCurrent,
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Home Device Manager'),
      ),
      body: GridView.builder(
        padding: const EdgeInsets.all(16.0),
        gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
          crossAxisCount: 1,
          crossAxisSpacing: 16.0,
          mainAxisSpacing: 16.0,
          childAspectRatio: 1.4,
        ),
        itemCount: _devices.length,
        itemBuilder: (context, index) {
          return DeviceWidget(
            device: _devices[index],
            onToggle: () => _toggleDevice(index),
          );
        },
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _showTestDialog,
        child: const Icon(Icons.science),
        tooltip: 'Test Device',
      ),
    );
  }
}
