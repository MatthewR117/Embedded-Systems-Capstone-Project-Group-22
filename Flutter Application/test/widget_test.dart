import 'package:capstone/main.dart';
import 'package:capstone/test_dialog.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('Device card smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const MyApp());

    // Verify that the app bar title is correct.
    expect(find.text('Home Device Manager'), findsOneWidget);

    // Verify that the first device is present.
    expect(find.text('lamp00001'), findsOneWidget);

    // Verify that the first device is on and the button says "Shutdown".
    expect(find.widgetWithText(ElevatedButton, 'Shutdown'), findsWidgets);

    // Tap the "Shutdown" button on the first device.
    await tester.tap(find.widgetWithText(ElevatedButton, 'Shutdown').first);
    await tester.pump();

    // Verify that the button text has changed to "Power On".
    expect(find.widgetWithText(ElevatedButton, 'Power On'), findsWidgets);
  });

  testWidgets('Test dialog updates voltage', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const MyApp());

    // Tap the floating action button.
    await tester.tap(find.byType(FloatingActionButton));
    await tester.pump();

    // Verify that the dialog is shown.
    expect(find.byType(TestDialog), findsOneWidget);

    // Tap the "Increase Voltage by 10" button.
    await tester.tap(find.text('Increase Voltage by 10'));
    await tester.pump();

    // Verify that the voltage of the first device has been updated.
    expect(find.text('Voltage: 130.5V'), findsOneWidget);
  });

  testWidgets('Card border color changes based on power state', (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());

    // Helper to get border color from Card
    Color getBorderColor(Card card) {
      final shape = card.shape as RoundedRectangleBorder;
      return shape.side.color;
    }

    // tv00001 is off, so its border should be blue
    final tvCard = tester.widget<Card>(find.ancestor(of: find.text('tv00001'), matching: find.byType(Card)));
    expect(getBorderColor(tvCard), Colors.blue);

    // lamp00001 is on with low power, so its border should be green
    final lampCard = tester.widget<Card>(find.ancestor(of: find.text('lamp00001'), matching: find.byType(Card)));
    expect(getBorderColor(lampCard), Colors.green);

    // computer00001 is on with high power, so its border should be red
    final computerCard = tester.widget<Card>(find.ancestor(of: find.text('computer00001'), matching: find.byType(Card)));
    expect(getBorderColor(computerCard), Colors.red);
  });
}
