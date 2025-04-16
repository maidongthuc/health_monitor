import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'dart:convert';
import 'user_profile_page.dart';
import 'change_pass_page.dart';
import 'welcom_page.dart';

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  String fullName = '';
  bool isConnected = false;
  late MqttServerClient client;

  // Dữ liệu sức khỏe
  double temperature = 0.0;
  int heartRate = 0;
  int spo2 = 0;
  int sys = 0;
  int dia = 0;
  String lastUpdated = 'Chưa cập nhật';

  // Lịch sử đo
  List<Map<String, dynamic>> measurementHistory = [];

  @override
  void initState() {
    super.initState();
    _loadUserData();
    _loadMeasurementHistory();
    _connectToMqtt();
  }

  Future<void> _loadUserData() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    setState(() {
      fullName = prefs.getString('fullName') ?? 'Người dùng';
    });
  }

  Future<void> _loadMeasurementHistory() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    String? historyJson = prefs.getString('measurementHistory');
    if (historyJson != null) {
      setState(() {
        measurementHistory =
            List<Map<String, dynamic>>.from(jsonDecode(historyJson));
      });
    }
  }

  Future<void> _saveMeasurementHistory() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    await prefs.setString('measurementHistory', jsonEncode(measurementHistory));
  }

  Future<void> _connectToMqtt() async {
    client = MqttServerClient(
        '42cb8a3135f84357959ce239305850c0.s1.eu.hivemq.cloud',
        'flutter_client');
    client.port = 8883;
    client.secure = true;
    client.logging(on: false);
    client.setProtocolV311();
    client.keepAlivePeriod = 20;

    client.onConnected = () {
      setState(() {
        isConnected = true;
      });
      debugPrint('Connected to MQTT broker');
      client.subscribe('health/data', MqttQos.atLeastOnce);
    };

    client.onDisconnected = () {
      setState(() {
        isConnected = false;
      });
      debugPrint('Disconnected from MQTT broker');
    };

    client.onSubscribed = (String topic) {
      debugPrint('Subscribed to $topic');
    };

    client.updates?.listen((List<MqttReceivedMessage<MqttMessage>> c) {
      final MqttPublishMessage message = c[0].payload as MqttPublishMessage;
      final payload =
          MqttPublishPayload.bytesToStringAsString(message.payload.message);
      debugPrint('Received message: $payload from topic: ${c[0].topic}');

      try {
        final data = jsonDecode(payload);
        final now = DateTime.now();
        setState(() {
          temperature = (data['temp'] as num).toDouble();
          spo2 = (data['spo2'] as num).toInt();
          heartRate = (data['hr'] as num).toInt();
          sys = (data['sys'] as num).toInt();
          dia = (data['dia'] as num).toInt();
          lastUpdated =
              '${now.hour}:${now.minute}:${now.second} ${now.day}/${now.month}/${now.year}';

          // Lưu vào lịch sử đo
          measurementHistory.insert(0, {
            'time': lastUpdated,
            'temperature': temperature,
            'heartRate': heartRate,
            'spo2': spo2,
            'sys': sys,
            'dia': dia,
          });
          if (measurementHistory.length > 10) {
            measurementHistory = measurementHistory.sublist(0, 10);
          }
        });
        _saveMeasurementHistory();
      } catch (e) {
        debugPrint('Error parsing JSON: $e');
      }
    });

    try {
      await client.connect('smarthome', 'Smarthome2023');
    } catch (e) {
      debugPrint('Exception: $e');
      client.disconnect();
    }
  }

  @override
  void dispose() {
    client.disconnect();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        leading: Builder(
          builder: (context) => IconButton(
            icon: Icon(
              Icons.menu,
              color: Colors.white,
              shadows: [
                Shadow(
                  color: Colors.black.withOpacity(0.5),
                  offset: Offset(1, 1),
                  blurRadius: 3,
                ),
              ],
            ),
            onPressed: () {
              Scaffold.of(context).openDrawer();
            },
          ),
        ),
        actions: [
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 10.0),
            child: Row(
              children: [
                Text(
                  fullName,
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: 16,
                    shadows: [
                      Shadow(
                        color: Colors.black.withOpacity(0.5),
                        offset: Offset(1, 1),
                        blurRadius: 3,
                      ),
                    ],
                  ),
                ),
                SizedBox(width: 10),
                CircleAvatar(
                  radius: 18,
                  backgroundColor: Colors.white,
                  child: Icon(Icons.person, color: Colors.blue[800]),
                ),
              ],
            ),
          ),
        ],
      ),
      drawer: Drawer(
        child: ListView(
          padding: EdgeInsets.zero,
          children: [
            DrawerHeader(
              decoration: BoxDecoration(
                color: Colors.blue[800],
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  CircleAvatar(
                    radius: 40,
                    backgroundColor: Colors.white,
                    child:
                        Icon(Icons.person, size: 50, color: Colors.blue[800]),
                  ),
                  SizedBox(height: 10),
                  Text(
                    fullName,
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ],
              ),
            ),
            ListTile(
              leading: Icon(Icons.person),
              title: Text('Thông tin cá nhân'),
              onTap: () {
                Navigator.pop(context);
                Navigator.push(
                  context,
                  MaterialPageRoute(builder: (context) => PersonalInfoScreen()),
                );
              },
            ),
            ListTile(
              leading: Icon(Icons.lock),
              title: Text('Thay đổi mật khẩu'),
              onTap: () {
                Navigator.pop(context);
                Navigator.push(
                  context,
                  MaterialPageRoute(
                      builder: (context) => ChangePasswordScreen()),
                );
              },
            ),
            ListTile(
              leading: Icon(Icons.logout),
              title: Text('Đăng xuất'),
              onTap: () async {
                SharedPreferences prefs = await SharedPreferences.getInstance();
                await prefs.clear();
                Navigator.pushReplacement(
                  context,
                  MaterialPageRoute(builder: (context) => WelcomeScreen()),
                );
              },
            ),
          ],
        ),
      ),
      body: Stack(
        children: [
          Positioned.fill(
            child: Image.asset(
              'lib/assets/images/welcome_bg.png',
              fit: BoxFit.cover,
            ),
          ),
          Positioned.fill(
            child: Container(
              color: Colors.black.withAlpha(127),
            ),
          ),
          SingleChildScrollView(
            physics: const BouncingScrollPhysics(),
            child: Padding(
              padding:
                  const EdgeInsets.symmetric(horizontal: 20.0, vertical: 40.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  SizedBox(height: 50),
                  // Dashboard sức khỏe
                  Text(
                    'Dashboard sức khỏe',
                    style: TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                    ),
                  ),
                  SizedBox(height: 5),
                  Text(
                    'Cập nhật lúc: $lastUpdated',
                    style: TextStyle(
                      fontSize: 14,
                      color: Colors.white70,
                      fontStyle: FontStyle.italic,
                    ),
                  ),
                  SizedBox(height: 5), // Giảm khoảng cách
                  GridView.count(
                    crossAxisCount: 2,
                    shrinkWrap: true,
                    physics: NeverScrollableScrollPhysics(),
                    crossAxisSpacing: 10,
                    mainAxisSpacing: 10,
                    childAspectRatio: 1,
                    children: [
                      // Nhiệt độ
                      _buildHealthCard(
                        Icons.thermostat,
                        'Nhiệt độ',
                        temperature == 0.0
                            ? 'Đang chờ...'
                            : '${temperature.toStringAsFixed(1)} °C',
                        Colors.teal[100]!,
                      ),
                      // Nhịp tim
                      _buildHealthCard(
                        Icons.favorite,
                        'Nhịp tim',
                        heartRate == 0 ? 'Đang chờ...' : '$heartRate bpm',
                        Colors.pink[100]!,
                      ),
                      // SpO2
                      _buildHealthCard(
                        Icons.opacity,
                        'SpO2',
                        spo2 == 0 ? 'Đang chờ...' : '$spo2%',
                        Colors.blue[100]!,
                      ),
                      // Huyết áp
                      _buildHealthCard(
                        Icons.arrow_upward,
                        'Huyết áp',
                        sys == 0 && dia == 0 ? 'Đang chờ...' : '$sys/$dia mmHg',
                        Colors.purple[100]!,
                      ),
                    ],
                  ),
                  SizedBox(height: 20),
                  // Lịch sử đo
                  Text(
                    'Lịch sử đo',
                    style: TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                    ),
                  ),
                  SizedBox(height: 10),
                  measurementHistory.isEmpty
                      ? Text(
                          'Chưa có dữ liệu lịch sử',
                          style: TextStyle(
                            fontSize: 16,
                            color: Colors.white70,
                          ),
                        )
                      : ListView.builder(
                          shrinkWrap: true,
                          physics: NeverScrollableScrollPhysics(),
                          itemCount: measurementHistory.length,
                          itemBuilder: (context, index) {
                            final record = measurementHistory[index];
                            return Card(
                              color: Colors.white.withOpacity(0.9),
                              margin: EdgeInsets.symmetric(vertical: 5),
                              child: Padding(
                                padding: const EdgeInsets.all(10.0),
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Text(
                                      'Thời gian: ${record['time']}',
                                      style: TextStyle(
                                        fontSize: 14,
                                        fontWeight: FontWeight.bold,
                                        color: Colors.black,
                                      ),
                                    ),
                                    SizedBox(height: 5),
                                    Text(
                                        'Nhiệt độ: ${record['temperature'].toStringAsFixed(1)} °C'),
                                    Text(
                                        'Nhịp tim: ${record['heartRate']} bpm'),
                                    Text('SpO2: ${record['spo2']}%'),
                                    Text(
                                        'Huyết áp: ${record['sys']}/${record['dia']} mmHg'),
                                  ],
                                ),
                              ),
                            );
                          },
                        ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHealthCard(
      IconData icon, String label, String value, Color bgColor) {
    return Card(
      color: bgColor,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(15),
      ),
      child: Padding(
        padding: const EdgeInsets.all(15.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, color: Colors.black54, size: 20),
                SizedBox(width: 5),
                Text(
                  label,
                  style: TextStyle(
                    fontSize: 16,
                    color: Colors.black54,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ],
            ),
            SizedBox(height: 10),
            Text(
              value,
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: Colors.black87,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
