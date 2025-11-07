import 'package:capstone/device_model.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';

class DeviceWidget extends StatelessWidget {
  final Device device;
  final VoidCallback onToggle;

  const DeviceWidget({
    super.key,
    required this.device,
    required this.onToggle,
  });

  Color _getBorderColor() {
    if (!device.isOn) {
      return Colors.blue;
    }
    final projectedKwh = (device.voltage * device.current * 8) / 1000;
    if (projectedKwh < 5) {
      return Colors.green;
    } else {
      return Colors.red;
    }
  }

  @override
  Widget build(BuildContext context) {
    final projectedKwh = (device.voltage * device.current * 8) / 1000;

    return Card(
      shape: RoundedRectangleBorder(
        side: BorderSide(color: _getBorderColor(), width: 2),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              device.name,
              style: const TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 120,
              child: LineChart(
                LineChartData(
                  gridData: const FlGridData(show: false),
                  titlesData: const FlTitlesData(show: false),
                  borderData: FlBorderData(
                    show: true,
                    border: Border.all(color: Colors.grey[800]!, width: 1),
                  ),
                  minX: 0,
                  maxX: 7,
                  minY: 0,
                  maxY: 200, // Adjust this based on expected max voltage/current
                  lineBarsData: [
                    _buildLineChartBarData(
                      color: Colors.lightBlueAccent,
                      data: _generateSampleData(device.voltage),
                    ),
                    _buildLineChartBarData(
                      color: Colors.redAccent,
                      data: _generateSampleData(device.current),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Projected 8-hour usage: ${projectedKwh.toStringAsFixed(2)} kWh',
            ),
            const SizedBox(height: 16),
            Wrap(
              spacing: 8.0,
              runSpacing: 4.0,
              alignment: WrapAlignment.spaceBetween,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Voltage: ${device.voltage.toStringAsFixed(1)}V',
                      style: const TextStyle(fontSize: 12),
                    ),
                    Text(
                      'Current: ${device.current.toStringAsFixed(1)}A',
                      style: const TextStyle(fontSize: 12),
                    ),
                  ],
                ),
                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    textStyle: const TextStyle(fontSize: 12),
                    padding:
                    const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                  ),
                  onPressed: onToggle,
                  child: Text(device.isOn ? 'Shutdown' : 'Power On'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  LineChartBarData _buildLineChartBarData({
    required Color color,
    required List<FlSpot> data,
  }) {
    return LineChartBarData(
      spots: data,
      isCurved: true,
      color: color,
      barWidth: 2,
      isStrokeCapRound: true,
      dotData: const FlDotData(show: false),
      belowBarData: BarAreaData(show: false),
    );
  }

  // This is just a sample data generator. In a real app, you would get this data from the microcontroller.
  List<FlSpot> _generateSampleData(double value) {
    return List.generate(8, (index) {
      final y = value + (index % 2 == 0 ? index * 2 : -index * 2);
      return FlSpot(index.toDouble(), y);
    });
  }
}
