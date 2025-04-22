import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'home_page.dart';

class UserProfileSetupScreen extends StatefulWidget {
  const UserProfileSetupScreen({super.key});

  @override
  UserProfileSetupScreenState createState() => UserProfileSetupScreenState();
}

class UserProfileSetupScreenState extends State<UserProfileSetupScreen> {
  final _fullNameController = TextEditingController();
  final _heightController = TextEditingController();
  final _weightController = TextEditingController();

  String? _selectedGender;
  final List<String> _genders = ['Nam', 'Nữ', 'Khác'];

  int? _selectedDay;
  int? _selectedMonth;
  int? _selectedYear;
  final List<int> _days = List.generate(31, (index) => index + 1);
  final List<int> _months = List.generate(12, (index) => index + 1);
  final List<int> _years =
      List.generate(100, (index) => DateTime.now().year - index);

  @override
  void dispose() {
    _fullNameController.dispose();
    _heightController.dispose();
    _weightController.dispose();
    super.dispose();
  }

  void _saveProfile() async {
    String fullName = _fullNameController.text;
    String? gender = _selectedGender;
    String height = _heightController.text;
    String weight = _weightController.text;

    if (fullName.isEmpty ||
        gender == null ||
        _selectedDay == null ||
        _selectedMonth == null ||
        _selectedYear == null ||
        height.isEmpty ||
        weight.isEmpty) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Vui lòng nhập đầy đủ thông tin')),
      );
    } else {
      // Lưu thông tin cá nhân vào SharedPreferences
      SharedPreferences prefs = await SharedPreferences.getInstance();
      await prefs.setString('fullName', fullName);
      await prefs.setString('gender', gender);
      await prefs.setInt('birthDay', _selectedDay!);
      await prefs.setInt('birthMonth', _selectedMonth!);
      await prefs.setInt('birthYear', _selectedYear!);
      await prefs.setString('height', height);
      await prefs.setString('weight', weight);

      debugPrint("Lưu thông tin: Họ và tên: $fullName, Giới tính: $gender, "
          "Ngày sinh: $_selectedDay/$_selectedMonth/$_selectedYear, "
          "Chiều cao: $height cm, Cân nặng: $weight kg");

      // Chuyển hướng đến HomePage
      if (!mounted) return;
      Navigator.pushReplacement(
        context,
        MaterialPageRoute(builder: (context) => HomePage()),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: Text(
          'Cấu hình thông tin',
          style: TextStyle(
            color: Colors.white,
            shadows: [
              Shadow(
                color: Colors.black.withAlpha(127),
                offset: Offset(1, 1),
                blurRadius: 3,
              ),
            ],
          ),
        ),
        backgroundColor: Colors.transparent,
        elevation: 0,
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
                mainAxisAlignment: MainAxisAlignment.center,
                crossAxisAlignment: CrossAxisAlignment.center,
                children: [
                  SizedBox(height: 50),
                  Row(
                    children: [
                      Expanded(
                        flex: 2,
                        child: TextField(
                          controller: _fullNameController,
                          decoration: InputDecoration(
                            labelText: 'Họ và tên',
                            labelStyle: TextStyle(color: Colors.black),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(10),
                            ),
                            prefixIcon:
                                Icon(Icons.person, color: Colors.blue[800]),
                            filled: true,
                            fillColor: Colors.white.withAlpha(229),
                          ),
                          style: TextStyle(color: Colors.black),
                        ),
                      ),
                      SizedBox(width: 10),
                      Expanded(
                        flex: 1,
                        child: DropdownButtonFormField<String>(
                          value: _selectedGender,
                          decoration: InputDecoration(
                            labelText: 'Giới tính',
                            labelStyle: TextStyle(color: Colors.black),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(10),
                            ),
                            prefixIcon: Icon(Icons.wc, color: Colors.blue[800]),
                            filled: true,
                            fillColor: Colors.white.withAlpha(229),
                          ),
                          items: _genders.map((String gender) {
                            return DropdownMenuItem<String>(
                              value: gender,
                              child: Text(gender,
                                  style: TextStyle(color: Colors.black)),
                            );
                          }).toList(),
                          onChanged: (String? newValue) {
                            setState(() {
                              _selectedGender = newValue;
                            });
                          },
                          dropdownColor: Colors.white,
                        ),
                      ),
                    ],
                  ),
                  SizedBox(height: 20),
                  Row(
                    children: [
                      Expanded(
                        child: DropdownButtonFormField<int>(
                          value: _selectedDay,
                          decoration: InputDecoration(
                            labelText: 'Ngày',
                            labelStyle: TextStyle(color: Colors.black),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(10),
                            ),
                            prefixIcon: Icon(Icons.calendar_today,
                                color: Colors.blue[800]),
                            filled: true,
                            fillColor: Colors.white.withAlpha(229),
                          ),
                          items: _days.map((int day) {
                            return DropdownMenuItem<int>(
                              value: day,
                              child: Text(day.toString(),
                                  style: TextStyle(color: Colors.black)),
                            );
                          }).toList(),
                          onChanged: (int? newValue) {
                            setState(() {
                              _selectedDay = newValue;
                            });
                          },
                          dropdownColor: Colors.white,
                        ),
                      ),
                      SizedBox(width: 10),
                      Expanded(
                        child: DropdownButtonFormField<int>(
                          value: _selectedMonth,
                          decoration: InputDecoration(
                            labelText: 'Tháng',
                            labelStyle: TextStyle(color: Colors.black),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(10),
                            ),
                            prefixIcon: Icon(Icons.calendar_today,
                                color: Colors.blue[800]),
                            filled: true,
                            fillColor: Colors.white.withAlpha(229),
                          ),
                          items: _months.map((int month) {
                            return DropdownMenuItem<int>(
                              value: month,
                              child: Text(month.toString(),
                                  style: TextStyle(color: Colors.black)),
                            );
                          }).toList(),
                          onChanged: (int? newValue) {
                            setState(() {
                              _selectedMonth = newValue;
                            });
                          },
                          dropdownColor: Colors.white,
                        ),
                      ),
                      SizedBox(width: 10),
                      Expanded(
                        child: DropdownButtonFormField<int>(
                          value: _selectedYear,
                          decoration: InputDecoration(
                            labelText: 'Năm',
                            labelStyle: TextStyle(color: Colors.black),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(10),
                            ),
                            prefixIcon: Icon(Icons.calendar_today,
                                color: Colors.blue[800]),
                            filled: true,
                            fillColor: Colors.white.withAlpha(127),
                          ),
                          items: _years.map((int year) {
                            return DropdownMenuItem<int>(
                              value: year,
                              child: Text(year.toString(),
                                  style: TextStyle(color: Colors.black)),
                            );
                          }).toList(),
                          onChanged: (int? newValue) {
                            setState(() {
                              _selectedYear = newValue;
                            });
                          },
                          dropdownColor: Colors.white,
                        ),
                      ),
                    ],
                  ),
                  SizedBox(height: 20),
                  Row(
                    children: [
                      Expanded(
                        child: TextField(
                          controller: _heightController,
                          decoration: InputDecoration(
                            labelText: 'Chiều cao (cm)',
                            labelStyle: TextStyle(color: Colors.black),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(10),
                            ),
                            prefixIcon:
                                Icon(Icons.height, color: Colors.blue[800]),
                            filled: true,
                            fillColor: Colors.white.withAlpha(127),
                          ),
                          keyboardType: TextInputType.number,
                          style: TextStyle(color: Colors.black),
                        ),
                      ),
                      SizedBox(width: 10),
                      Expanded(
                        child: TextField(
                          controller: _weightController,
                          decoration: InputDecoration(
                            labelText: 'Cân nặng (kg)',
                            labelStyle: TextStyle(color: Colors.black),
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(10),
                            ),
                            prefixIcon: Icon(Icons.fitness_center,
                                color: Colors.blue[800]),
                            filled: true,
                            fillColor: Colors.white.withAlpha(127),
                          ),
                          keyboardType: TextInputType.number,
                          style: TextStyle(color: Colors.black),
                        ),
                      ),
                    ],
                  ),
                  SizedBox(height: 30),
                  ElevatedButton(
                    onPressed: _saveProfile,
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.blue[800],
                      padding:
                          EdgeInsets.symmetric(horizontal: 50, vertical: 15),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(25),
                      ),
                      elevation: 5,
                    ),
                    child: Text(
                      'LƯU',
                      style: TextStyle(
                        fontSize: 16,
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ),
                  SizedBox(height: 30),
                  Text(
                    'Hãy điền thông tin để chúng tôi hỗ trợ bạn tốt hơn!',
                    style: TextStyle(
                      fontSize: 16,
                      color: Colors.white,
                      fontStyle: FontStyle.italic,
                      shadows: [
                        Shadow(
                          color: Colors.black.withAlpha(127),
                          offset: Offset(1, 1),
                          blurRadius: 3,
                        ),
                      ],
                    ),
                    textAlign: TextAlign.center,
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}
