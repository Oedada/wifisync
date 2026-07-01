import 'dart:io';

class Player {
  String? name;
  int hp = 100;
  int stamina = 100;
  Player({required this.name, this.hp = 100, this.stamina = 100});
  void takeDamage(int damage) {
    hp -= damage;
  }

  int getHealth() {
    return hp;
  }

}

void main() {
  var input = stdin.readLineSync();
  if (input == null) {
    return;
  }
  if (int.parse(input) % 2 == 1) {
    print("Нечетная");
  } else {
    print("Чётное");
  }
}
