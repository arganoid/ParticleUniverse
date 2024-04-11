#pragma once

#include <memory>
#include <functional>
#include <sstream>
#include <string>
#include <memory>
#include <string>
#include <variant>
#include <vector>


struct RGB;

using DialogTextFunc = std::function<std::string()>;
using DialogTextSource = std::variant<std::string, DialogTextFunc>;

class CmdButtonBase
{
public:
	int x1, y1, x2, y2;
	RGB* col = nullptr;

	CmdButtonBase(int x1, int y1, int x2, int y2) :
		x1(x1),
		x2(x2),
		y1(y1),
		y2(y2)
	{
	}

	virtual std::string getText() const = 0;
	virtual std::string valueStr() const { return "";  };
	virtual void select() = 0;
	virtual void left() {};
	virtual void right() {};
	virtual void deletePressed() {};
	virtual bool isHeading() const { return false; }

	void setCol(RGB* col)
	{
		this->col = col;
	}

	RGB* getCol() const
	{
		return col;
	}
};

class CmdButtonText : public CmdButtonBase
{
	DialogTextSource text;
public:

	CmdButtonText(int x1, int y1, int x2, int y2, DialogTextSource text) :
		CmdButtonBase(x1,y1,x2,y2),
		text(text)
	{
	}

	virtual std::string getText() const override
	{
		if (std::holds_alternative<std::string>(text))
			return std::get<std::string>(text);
		else
			return std::get<DialogTextFunc>(text)();
	}

	virtual void setText(std::string _t) { text = _t; }
};

class CmdButtonInt : public CmdButtonText
{
	int minValue, maxValue;
	int step;
protected:
	int* value;
public:
	CmdButtonInt(int x1, int y1, int x2, int y2, std::string text, int* value, int minValue, int maxValue, int step = 1) :
		CmdButtonText(x1,y1,x2,y2,text),
		value(value),
		minValue(minValue),
		maxValue(maxValue),
		step(step)
	{}

	virtual std::string valueStr() const override
	{
		std::ostringstream s;
		s << *value;
		return s.str();
	}
	
	void select() override
	{
		*value += step;
		if (*value > maxValue)
		{
			*value = minValue;
		}
	}

	void left() override
	{
		*value -= step;
		if (*value < minValue)
		{
			*value = maxValue;
		}
	}

	void right() override { select();  }

};

class CmdButtonEnum : public CmdButtonInt
{
	std::shared_ptr<std::vector<std::string>> toStrLookup;
public:
	CmdButtonEnum(int x1, int y1, int x2, int y2, std::string text, int* value, int minValue, int maxValue, std::shared_ptr<std::vector<std::string>> toStrLookup) :
		CmdButtonInt(x1, y1, x2, y2, text, value, minValue, maxValue),
		toStrLookup(toStrLookup)
	{}

	std::string valueStr() const override
	{
		return (*toStrLookup)[*value];
	}
};

class CmdButtonBool : public CmdButtonText
{
	bool* value;
	std::shared_ptr<std::vector<std::string>> toStrLookup;
public:
	CmdButtonBool(int x1, int y1, int x2, int y2, std::string text, bool* value, std::shared_ptr<std::vector<std::string>> toStrLookup) :
		CmdButtonText(x1, y1, x2, y2, text),
		value(value),
		toStrLookup(toStrLookup)
	{}

	void select() override
	{
		*value = !*value;
	}

	void left() override { select(); }
	void right() override { select(); }

	std::string valueStr() const override
	{
		return (*toStrLookup)[(int)*value];
	}
};

class CmdButtonAction : public CmdButtonText
{
	std::function<void(CmdButtonAction&)> action;
	std::function<void(CmdButtonAction&)> deleteAction;
	std::function<void(CmdButtonAction&)> leftAction;
	std::function<void(CmdButtonAction&)> rightAction;
public:
	CmdButtonAction(int x1, int y1, int x2, int y2, DialogTextSource text, std::function<void(CmdButtonAction&)> action, std::function<void(CmdButtonAction&)> deleteAction = nullptr) :
		CmdButtonText(x1, y1, x2, y2, text),
		action(action),
		deleteAction(deleteAction)
	{}

	void select() override
	{
		if (action)
			action(*this);
	}

	void deletePressed() override
	{
		if (deleteAction)
			deleteAction(*this);
	}

	void left() override
	{
		if (leftAction)
			leftAction(*this);
	}

	void right() override
	{
		if (rightAction)
			rightAction(*this);
	}

	void setAction(std::function<void(CmdButtonAction&)> _action)
	{
		action = _action;
	}

	void setDeleteAction(std::function<void(CmdButtonAction&)> _deleteAction)
	{
		deleteAction = _deleteAction;
	}

	void setLeftAction(std::function<void(CmdButtonAction&)> _leftAction)
	{
		leftAction = _leftAction;
	}

	void setRightAction(std::function<void(CmdButtonAction&)> _rightAction)
	{
		rightAction = _rightAction;
	}
};

class CmdButtonDynamic : public CmdButtonBase
{
	std::function<std::string()> text;
	std::function<std::string()> value;
	std::function<void()> action;
	std::function<void()> leftAction;
	std::function<void()> rightAction;
public:
	CmdButtonDynamic(int x1, int y1, int x2, int y2, std::function<std::string()> text, std::function<std::string()> value,
					std::function<void()> action, std::function<void()> leftAction = [] {}, std::function<void()> rightAction = [] {}) :
		CmdButtonBase(x1, y1, x2, y2),
		text(text),
		value(value),
		action(action),
		leftAction(leftAction),
		rightAction(rightAction)
	{
	}

	std::string getText() const override
	{
		return text();
	}
	
	std::string valueStr() const override
	{
		return value();
	}

	void select() override
	{
		action();
	}

	void left() override
	{
		leftAction();
	}

	void right() override
	{
		rightAction();
	}
};

class CmdButtonHeading : public CmdButtonText
{
	std::string text;
public:

	CmdButtonHeading(int x1, int y1, int x2, int y2, std::string text) :
		CmdButtonText(x1, y1, x2, y2, text)
	{
	}

	virtual bool isHeading() const { return true; }
	virtual void select() {}
};

//template<typename T> class CmdButton : public CmdButtonBase
//{
//public:
//	T value;
//	string valueStr()
//	{
//		std::ostringstream s;
//		s << value;
//		return s.str();
//	}
//};

class CmdMenu
{
	int focusIndex = 0;
	
	void move(int dir)
	{
		size_t i = 0;
		do
		{
			focusIndex += dir;

			// Check that new focusIndex exists
			if (focusIndex >= (int)buttons.size())
				focusIndex = 0;
			else if (focusIndex < 0)
				focusIndex = (int)buttons.size() - 1;
		}
		while (buttons[focusIndex]->isHeading() || ++i >= buttons.size());
	}

public:

	std::vector<std::shared_ptr<CmdButtonBase>> buttons;

    void movedown ()
    {
		move(1);
    }

    void moveup ()
    {
		move(-1);
    }

	void select()
	{
		auto& button = buttons[focusIndex];
		button->select();
	}

	void left(void)
	{
		auto& button = buttons[focusIndex];
		button->left();
	}

	void right(void)
	{
		auto& button = buttons[focusIndex];
		button->right();
	}
    void setfocus (int target)
    {   
		focusIndex = target;
    }

	void deletePressed()
	{
		auto& button = buttons[focusIndex];
		button->deletePressed();
	}

	void add(std::shared_ptr<CmdButtonBase> button)
	{
		buttons.push_back(button);
	}

    void addInt (int x1, int y1, int x2, int y2, std::string const& text, int* value, int minValue, int maxValue, int step = 1)
    {
		buttons.push_back( std::make_shared<CmdButtonInt>( x1,y1,x2,y2,text,value, minValue, maxValue, step) );
    }

	void addEnum(int x1, int y1, int x2, int y2, std::string const& text, int* value, int minValue, int maxValue, std::shared_ptr<std::vector<std::string>> toStrLookup)
	{
		buttons.push_back(std::make_shared<CmdButtonEnum>(x1, y1, x2, y2, text, value, minValue, maxValue, toStrLookup));
	}

	void addBool(int x1, int y1, int x2, int y2, std::string const& text, bool* value, std::shared_ptr<std::vector<std::string>> toStrLookup)
	{
		buttons.push_back(std::make_shared<CmdButtonBool>(x1, y1, x2, y2, text, value, toStrLookup));
	}

	std::shared_ptr<CmdButtonAction> addAction(int x1, int y1, int x2, int y2, DialogTextSource text, std::function<void(CmdButtonAction&)> action, std::function<void(CmdButtonAction&)> deleteAction = nullptr)
	{
		auto button = std::make_shared<CmdButtonAction>(x1, y1, x2, y2, text, action, deleteAction);
		buttons.push_back(button);
		return button;
	}

	void addHeading(int x1, int y1, int x2, int y2, std::string const& text)
	{
		buttons.push_back(std::make_shared<CmdButtonHeading>(x1, y1, x2, y2, text));
	}

	//template<typename T> void add(int x1, int y1, int x2, int y2, std::string const& text, T value, std::function<string> toStr)
	//{
	//	buttons.push_back(CmdButton<T>{ x1, y1, x2, y2, text, toStr, value });
	//}

	int getFocusIndex() const
	{
		return focusIndex;
	}

	std::shared_ptr<CmdButtonBase> focusButton() const
	{
		return buttons[focusIndex];
	}
};
