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
};

class level01 : public level00 {
public:
	int var01;

	int GetVar01() const { return var01; }
	void SetVar01(int var) { var01 = var; }
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

	lua["print"] = [](const std::string& text)
	{
		std::cout << text << std::endl;
	};
 
	lua.new_usertype<level00>("level00",
		//"var00", &level00::var00,
		//"var00_prop", sol::property(&level00::GetVar00, &level00::SetVar00),
		"GetVar00", &level00::GetVar00,
		"SetVar00", &level00::SetVar00
	);

	lua.new_usertype<level01>("level01",
		//sol::call_constructor, sol::constructors<sol::types<>>(),
		sol::base_classes, sol::bases<level00>(),
		"GetVar01", &level01::GetVar01,
		"SetVar01", &level01::SetVar01
	);

	lua.new_usertype<level02>("level02",
		sol::base_classes, sol::bases<level00, level01>(),
		"GetVar02", &level02::GetVar02,
		"SetVar02", &level02::SetVar02
	);

	try
	{
		lua.script("x = level00.new()");
		lua.script("x:SetVar00(1)");
		lua.script("print(string.format('%d', x:GetVar00()))");

		lua.script("y = level01.new()");
		lua.script("y:SetVar01(2)");
		lua.script("print(string.format('%d', y:GetVar01()))");
		lua.script("y:SetVar00(3)");
		lua.script("print(string.format('%d', y:GetVar00()))");

		lua.script("z = level02.new()");
		lua.script("z:SetVar02(4)");
		lua.script("print(string.format('%d', z:GetVar02()))");
		lua.script("z:SetVar00(5)");
		lua.script("print(string.format('%d', z:GetVar00()))");
		lua.script("z:SetVar01(6)");
		lua.script("print(string.format('%d', z:GetVar01()))");
	}
	catch (std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
	}

	_getch();
	return 0;
}
