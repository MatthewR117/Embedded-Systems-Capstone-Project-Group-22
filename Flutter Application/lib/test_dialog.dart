import 'package:capstone/device_model.dart';
import 'package:flutter/material.dart';

class TestDialog extends StatefulWidget {
  final List<Device> devices;
  final Function(int) onVoltageUp;
  final Function(int) onCurrentUp;

  const TestDialog({
    super.key,
    required this.devices,
    required this.onVoltageUp,
    required this.onCurrentUp,
  });

  @override
  State<TestDialog> createState() => _TestDialogState();
}

class _TestDialogState extends State<TestDialog> {
  int _selectedIndex = 0;

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: const Text('Test Controls'),
      content: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          DropdownButton<int>(
            value: _selectedIndex,
            onChanged: (int? newValue) {
              if (newValue != null) {
                setState(() {
                  _selectedIndex = newValue;
                });
              }
            },
            items: widget.devices.asMap().entries.map((entry) {
              return DropdownMenuItem<int>(
                value: entry.key,
                child: Text(entry.value.name),
              );
            }).toList(),
          ),
          const SizedBox(height: 16),
          ElevatedButton(
            onPressed: () => widget.onVoltageUp(_selectedIndex),
            child: const Text('Increase Voltage by 10'),
          ),
          ElevatedButton(
            onPressed: () => widget.onCurrentUp(_selectedIndex),
            child: const Text('Increase Current by 10'),
          ),
        ],
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: const Text('Close'),
        ),
      ],
    );
  }
}
