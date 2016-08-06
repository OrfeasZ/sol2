#include <sol.hpp>
#include <iostream>
#include <cassert>
#include <cmath>
#include <conio.h>

class level00 {
public:
	int var00;

	int GetVar00() const { return var00; }
	void SetVar00(int var) { var00 = var; }
	virtual int Test() { return 123; }
};

class level01 : public level00 {
public:
	int var01;

	int GetVar01() const { return var01; }
	void SetVar01(int var) { var01 = var; }
	virtual int Test() override { return 456; }
};

class level02 : public level01 {
public:
	int var02;

	int GetVar02() const { return var02; }
	void SetVar02(int var) { var02 = var; }
};

int main() {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

	lua["print"] = [](const std::string& text) {
		std::cout << text << std::endl;
	};

	lua.new_usertype<level00>("level00",
		"var00", &level00::var00,
		"var00_prop", sol::property(&level00::GetVar00, &level00::SetVar00),
		"GetVar00", &level00::GetVar00,
		"SetVar00", &level00::SetVar00,
		"Test", &level00::Test
	);

	lua.new_usertype<level01>("level01",
		sol::base_classes, sol::bases<level00>(),
		"var01", &level01::var01,
		"GetVar01", &level01::GetVar01,
		"SetVar01", &level01::SetVar01
	);

	lua.new_usertype<level02>("level02",
		"var02", &level02::var02,
		"GetVar02", &level02::GetVar02,
		sol::base_classes, sol::bases<level00, level01>(),
		"SetVar02", &level02::SetVar02
	);

	try {
		// TODO: Turn these into actual tests and move them out of here.
		lua.script("x = level00.new()");
		lua.script("print(string.format('%d - expected 123', x:Test()))");
		lua.script("x:SetVar00(1)");
		lua.script("print(string.format('%d - expected 1', x:GetVar00()))");
		lua.script("x.var00 = 2");
		lua.script("print(string.format('%d - expected 2', x.var00))");

		lua.script("y = level01.new()");
		lua.script("print(string.format('%d - expected 456', y:Test()))");
		lua.script("y:SetVar01(3)");
		lua.script("print(string.format('%d - expected 3', y.var01))");
		lua.script("print(string.format('%d - expected 3', y:GetVar01()))");
		lua.script("y:SetVar00(4)");
		lua.script("print(string.format('%d - expected 4', y:GetVar00()))");
		lua.script("print(string.format('%d - expected 3', y:GetVar01()))");
		lua.script("print(string.format('%d - expected 4', y.var00))");
		lua.script("y.var01 = 5");
		lua.script("print(string.format('%d - expected 5', y.var01))");

		lua.script("z = level02.new()");
		lua.script("z:SetVar02(6)");
		lua.script("print(string.format('%d - expected 6', z.var02))");
		lua.script("print(string.format('%d - expected 6', z:GetVar02()))");
		lua.script("z:SetVar00(7)");
		lua.script("print(string.format('%d - expected 7', z:GetVar00()))");
		lua.script("print(string.format('%d - expected 0', z:GetVar01()))");
		lua.script("print(string.format('%d - expected 6', z:GetVar02()))");
		lua.script("print(string.format('%d - expected 7', z.var00))");
		lua.script("print(string.format('%d - expected 0', z.var01))");
		lua.script("z:SetVar01(8)");
		lua.script("print(string.format('%d - expected 7', z:GetVar00()))");
		lua.script("print(string.format('%d - expected 8', z:GetVar01()))");
		lua.script("print(string.format('%d - expected 6', z:GetVar02()))");
		lua.script("print(string.format('%d - expected 7', z.var00))");
		lua.script("print(string.format('%d - expected 8', z.var01))");
		lua.script("z.var02 = 9");
		lua.script("print(string.format('%d - expected 9', z.var02))");
		lua.script("print(string.format('%d - expected 7', z.var00_prop))");
		lua.script("z.var00_prop = 10");
		lua.script("print(string.format('%d - expected 10', z:GetVar00()))");

	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}

	_getch();
	return 0;
}
