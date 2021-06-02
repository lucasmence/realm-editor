#include <SFML/Graphics.hpp>
#include <memory>
#include "../model/model.hpp"
#include "../text/label.hpp"

#pragma once

#ifndef EDIT_HPP
#define EDIT_HPP

enum class EditType {etString, etInteger};

struct EditValue
{
	std::string string;
	int integer;
};

class Manager;

class Edit 
{
	public:
		Manager* manager;
		EditType type;
		EditValue value;
		std::shared_ptr<Model> shape;
		std::shared_ptr<Label> label;
		std::string name;
		bool selected;
		int maxLength;
		int integerMaxValue;
		int integerMinValue;

		Edit(Manager* manager,
			 EditType type,
			 std::string caption, 
			 sf::Vector2f position, 
			 std::string name, 
			 int size = 20, 
			 sf::FloatRect neighbor = sf::FloatRect(-1.f, -1.f, -1.f, -1.f),
			 sf::Vector2i side = sf::Vector2i(0, 0));
		~Edit();

		int getInt(std::string value);
		bool setValue(std::string value);
		EditValue getValue();
		bool updateLabel(std::string value);
		bool clear();
		bool setVisible(const bool value);

};

#endif