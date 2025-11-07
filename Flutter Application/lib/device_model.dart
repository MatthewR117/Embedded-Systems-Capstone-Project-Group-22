class Device {
  final String name;
  final String type;
  bool isOn;
  double voltage;
  double current;

  Device({
    required this.name,
    required this.type,
    required this.isOn,
    required this.voltage,
    required this.current,
  });
}
