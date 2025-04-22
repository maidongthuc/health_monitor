import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'dart:convert';
import 'user_profile_page.dart';
import 'change_pass_page.dart';
import 'welcom_page.dart';
import 'chatbot_page.dart';
import 'notification_page.dart';

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  HomePageState createState() => HomePageState();
}

class HomePageState extends State<HomePage> {
  int _selectedIndex = 0;
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
    if (!mounted) return;
    setState(() {
      fullName = prefs.getString('fullName') ?? 'Người dùng';
    });
  }

  Future<void> _loadMeasurementHistory() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    String? historyJson = prefs.getString('measurementHistory');
    if (!mounted) return;
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
      debugPrint('MQTT Client connected');
      client.subscribe('health/data', MqttQos.atLeastOnce);
      debugPrint('Subscribed to health/data');

      // Set up message handler
      client.updates!.listen(
        (List<MqttReceivedMessage<MqttMessage>> c) {
          // debugPrint('Got message in listener');
          final MqttPublishMessage recMess = c[0].payload as MqttPublishMessage;
          final String message =
              MqttPublishPayload.bytesToStringAsString(recMess.payload.message);
          // debugPrint('Message: $message');

          try {
            final data = jsonDecode(message);
            // debugPrint('Parsed JSON data: $data');
            if (!mounted) return;
            setState(() {
              temperature = (data['temp'] as num).toDouble();
              spo2 = (data['spo2'] as num).toInt();
              heartRate = (data['hr'] as num).toInt();
              sys = (data['sys'] as num).toInt();
              dia = (data['dia'] as num).toInt();
              final now = DateTime.now();
              lastUpdated =
                  '${now.hour}:${now.minute}:${now.second} ${now.day}/${now.month}/${now.year}';
            });

            // Add to measurement history
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
            _saveMeasurementHistory();
          } catch (e) {
            debugPrint('Error processing message: $e');
          }
        },
        onError: (error) {
          debugPrint('MQTT listener error: $error');
        },
        onDone: () {
          debugPrint('MQTT listener done');
        },
      );

      if (!mounted) return;
      setState(() {
        isConnected = true;
      });
    };

    client.onDisconnected = () {
      debugPrint('MQTT Client disconnected');
      if (!mounted) return;
      setState(() {
        isConnected = false;
      });
    };

    try {
      debugPrint('Connecting to MQTT broker...');
      await client.connect('smarthome', 'Smarthome2023');
    } catch (e) {
      debugPrint('Failed to connect to MQTT broker: $e');
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
              color: Colors.grey,
              shadows: [
                Shadow(
                  color: Colors.black.withAlpha(127),
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
                    color: Colors.grey,
                    fontSize: 16,
                    shadows: [
                      Shadow(
                        color: Colors.black.withAlpha(65),
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
                  child: Icon(Icons.person,
                      color: const Color.fromARGB(255, 67, 149, 243)),
                ),
              ],
            ),
          ),
        ],
      ),
      drawer: Drawer(
        child: Column(
          children: [
            DrawerHeader(
              decoration: BoxDecoration(
                color: Colors.blue[800],
                image: DecorationImage(
                  image: AssetImage('lib/assets/images/v870-tang-36.jpg'),
                  fit: BoxFit.cover,
                  colorFilter: ColorFilter.mode(
                    Colors.blueGrey.withAlpha(127),
                    BlendMode.darken,
                  ),
                ),
              ),
              child: SizedBox(
                width: double.infinity,
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.center,
                  mainAxisAlignment: MainAxisAlignment.center,
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
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                        shadows: [
                          Shadow(
                            color: Colors.black.withAlpha(64),
                            offset: Offset(1, 1),
                            blurRadius: 3,
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ),
            Expanded(
              child: Container(
                color: Colors.white,
                child: Column(
                  children: [
                    ListTile(
                      leading: Icon(Icons.person, color: Colors.blue[800]),
                      title: Text(
                        'Thông tin cá nhân',
                        style: TextStyle(
                          fontSize: 16,
                          color: Colors.black87,
                        ),
                      ),
                      onTap: () {
                        Navigator.pop(context);
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                              builder: (context) => PersonalInfoScreen()),
                        );
                      },
                    ),
                    ListTile(
                      leading: Icon(Icons.lock, color: Colors.blue[800]),
                      title: Text(
                        'Thay đổi mật khẩu',
                        style: TextStyle(
                          fontSize: 16,
                          color: Colors.black87,
                        ),
                      ),
                      onTap: () {
                        Navigator.pop(context);
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                              builder: (context) => ChangePasswordScreen()),
                        );
                      },
                    ),
                    Divider(height: 1, color: Colors.grey[300]),
                    ListTile(
                      leading: Icon(Icons.logout, color: Colors.red),
                      title: Text(
                        'Đăng xuất',
                        style: TextStyle(
                          fontSize: 16,
                          color: Colors.red,
                        ),
                      ),
                      onTap: () async {
                        SharedPreferences prefs =
                            await SharedPreferences.getInstance();
                        await prefs.clear();
                        if (!mounted) return;
                        Navigator.pushReplacement(
                          context,
                          MaterialPageRoute(
                              builder: (context) => WelcomeScreen()),
                        );
                      },
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
      body: IndexedStack(
        index: _selectedIndex,
        children: [
          HomeScreen(
            temperature: temperature,
            heartRate: heartRate,
            spo2: spo2,
            sys: sys,
            dia: dia,
            lastUpdated: lastUpdated,
            measurementHistory: measurementHistory,
          ),
          const ChatbotScreen(),
          const NotificationScreen(),
        ],
      ),
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _selectedIndex,
        onTap: (index) {
          setState(() {
            _selectedIndex = index;
          });
        },
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.home),
            label: 'Trang chủ',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.chat),
            label: 'Chatbot',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.notifications),
            label: 'Thông báo',
          ),
        ],
        selectedItemColor: Colors.blue[800],
        unselectedItemColor: Colors.grey,
      ),
    );
  }
}

class HomeScreen extends StatelessWidget {
  final double temperature;
  final int heartRate;
  final int spo2;
  final int sys;
  final int dia;
  final String lastUpdated;
  final List<Map<String, dynamic>> measurementHistory;

  const HomeScreen({
    super.key,
    required this.temperature,
    required this.heartRate,
    required this.spo2,
    required this.sys,
    required this.dia,
    required this.lastUpdated,
    required this.measurementHistory,
  });

  @override
  Widget build(BuildContext context) {
    final mediaQuery = MediaQuery.of(context);
    final topPadding = mediaQuery.padding.top;

    return Container(
      color: Colors.white,
      child: SingleChildScrollView(
        physics: const BouncingScrollPhysics(),
        child: Padding(
          padding: EdgeInsets.fromLTRB(15.0, topPadding, 15.0, 15.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Dashboard container with health cards
              Container(
                padding: EdgeInsets.all(15),
                decoration: BoxDecoration(
                  color: Colors.blue[50],
                  borderRadius: BorderRadius.circular(15),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Thông số sức khỏe',
                      style: TextStyle(
                        fontSize: 24,
                        fontWeight: FontWeight.bold,
                        color: Colors.blue[900],
                      ),
                    ),
                    SizedBox(height: 2),
                    Text(
                      'Cập nhật lúc: $lastUpdated',
                      style: TextStyle(
                        fontSize: 14,
                        color: Colors.blue[700],
                        fontStyle: FontStyle.italic,
                      ),
                    ),
                    GridView.count(
                      crossAxisCount: 2,
                      shrinkWrap: true,
                      physics: NeverScrollableScrollPhysics(),
                      crossAxisSpacing: 10,
                      mainAxisSpacing: 10,
                      childAspectRatio: 1.1,
                      children: [
                        _buildHealthCard(
                          Icons.thermostat,
                          'Nhiệt độ',
                          temperature == 0.0
                              ? '...'
                              : '${temperature.toStringAsFixed(1)} °C',
                          Colors.yellow[50]!,
                          Colors.yellow[800]!,
                        ),
                        _buildHealthCard(
                          Icons.favorite,
                          'Nhịp tim',
                          heartRate == 0 ? '...' : '$heartRate bpm',
                          Colors.pink[50]!,
                          Colors.pink[800]!,
                        ),
                        _buildHealthCard(
                          Icons.opacity,
                          'SpO2',
                          spo2 == 0 ? '...' : '$spo2%',
                          Colors.teal[50]!,
                          Colors.teal[800]!,
                        ),
                        _buildHealthCard(
                          Icons.bloodtype,
                          'Huyết áp',
                          sys == 0 && dia == 0 ? '...' : '$sys/$dia mmHg',
                          Colors.purple[50]!,
                          Colors.purple[800]!,
                        ),
                      ],
                    ),
                  ],
                ),
              ),
              SizedBox(height: 20),
              // History section
              Container(
                padding: EdgeInsets.all(15),
                decoration: BoxDecoration(
                  color: Colors.blue[50],
                  borderRadius: BorderRadius.circular(15),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Lịch sử đo',
                      style: TextStyle(
                        fontSize: 24,
                        fontWeight: FontWeight.bold,
                        color: Colors.blue[900],
                      ),
                    ),
                    SizedBox(height: 10),
                    _buildMeasurementHistory(),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHealthCard(IconData icon, String label, String value,
      Color bgColor, Color iconColor) {
    return Container(
      decoration: BoxDecoration(
        color: bgColor,
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
                Container(
                  padding: EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: iconColor.withAlpha(26),
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: Icon(icon, color: iconColor, size: 24),
                ),
                SizedBox(width: 10),
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
            SizedBox(height: 15),
            Text(
              value,
              style: TextStyle(
                fontSize: 18,
                fontWeight: FontWeight.bold,
                color: Colors.black87,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildMeasurementHistory() {
    if (measurementHistory.isEmpty) {
      return Container(
        padding: EdgeInsets.all(15),
        decoration: BoxDecoration(
          color: Colors.white,
          borderRadius: BorderRadius.circular(10),
        ),
        child: Center(
          child: Text(
            'Chưa có dữ liệu lịch sử',
            style: TextStyle(
              fontSize: 16,
              color: Colors.grey[600],
            ),
          ),
        ),
      );
    }
    return ListView.builder(
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
      itemCount: measurementHistory.length,
      itemBuilder: (context, index) {
        final record = measurementHistory[index];
        return Container(
          margin: EdgeInsets.only(bottom: 10),
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(10),
          ),
          child: Padding(
            padding: const EdgeInsets.all(15.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Icon(Icons.access_time, size: 16, color: Colors.blue[800]),
                    SizedBox(width: 5),
                    Text(
                      record['time'],
                      style: TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.bold,
                        color: Colors.blue[800],
                      ),
                    ),
                  ],
                ),
                SizedBox(height: 10),
                Row(
                  children: [
                    Expanded(
                      child: _buildHistoryItem(
                        Icons.thermostat,
                        '${record['temperature'].toStringAsFixed(1)} °C',
                        Colors.teal[800]!,
                      ),
                    ),
                    Expanded(
                      child: _buildHistoryItem(
                        Icons.favorite,
                        '${record['heartRate']} bpm',
                        Colors.pink[800]!,
                      ),
                    ),
                  ],
                ),
                SizedBox(height: 10),
                Row(
                  children: [
                    Expanded(
                      child: _buildHistoryItem(
                        Icons.opacity,
                        '${record['spo2']}%',
                        Colors.blue[800]!,
                      ),
                    ),
                    Expanded(
                      child: _buildHistoryItem(
                        Icons.arrow_upward,
                        '${record['sys']}/${record['dia']} mmHg',
                        Colors.purple[800]!,
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildHistoryItem(IconData icon, String value, Color color) {
    return Row(
      children: [
        Icon(icon, size: 16, color: color),
        SizedBox(width: 5),
        Text(
          value,
          style: TextStyle(
            fontSize: 14,
            color: Colors.black87,
          ),
        ),
      ],
    );
  }
}

class ChatbotScreen extends StatelessWidget {
  const ChatbotScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Text('Chatbot Screen'),
    );
  }
}

class NotificationScreen extends StatelessWidget {
  const NotificationScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Text('Notification Screen'),
    );
  }
}
