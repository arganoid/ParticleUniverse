#pragma once

#include "allegro5/allegro.h"

#include <string>
#include <sstream>
#include <optional>

class Config
{
public:
	static inline ALLEGRO_CONFIG* config = nullptr;
	static inline std::string configFilename;

	template<typename T>
	static void setValue(const char* section, const char* key, T value)
	{
		std::ostringstream ss;
		ss << value;
		al_set_config_value(config, section, key, ss.str().c_str());
		al_save_config_file(configFilename.c_str(), config);
	}

	template<>
	static void setValue<bool>(const char* section, const char* key, bool value)
	{
		setValue<int>(section, key, (int)value);
	}

	static bool doesValueExist(const char* section, const char* key)
	{
		const char* buf = al_get_config_value(config, section, key);
		return buf != nullptr;
	}

	template<typename T>
	static T getValue(const char* section, const char* key, T defaultValue)
	{
		const char* buf = al_get_config_value(config, section, key);
		if (!buf)
			return defaultValue;
		T val;
		std::istringstream iss(buf);
		iss >> val;
		return val;
	}

	template<>
	static bool getValue<bool>(const char* section, const char* key, bool defaultValue)
	{
		return (bool)getValue<int>(section, key, (int)defaultValue);
	}
};

class IConfigOptionWrapper
{
public:
	virtual void save() = 0;
	virtual void resetToDefault() = 0;
};

template<typename T>
class ConfigOptionWrapper : public IConfigOptionWrapper
{
public:
	// If defaultVal not specified, only load value from config if it exists
	ConfigOptionWrapper(const char* const& cat, const char* const& n, const char* displayName, std::optional<T> defaultVal, bool autoSave = true) :
		category(cat), name(n), displayName(displayName), defaultValue(defaultVal), autoSave(true)
	{}

	bool hasValue() const { return Config::doesValueExist(category, name); }

	T get()
	{
		// lazy eval, if option doesn't exist in file then we will use default value
		if (!value.has_value())
			load();
		return value.value();
	}

	void set(T val)
	{
		value = val;
		if (autoSave)
			save();
	}

	virtual void save()
	{
		Config::setValue<T>(category, name, get());
	}
	
	virtual void resetToDefault()
	{
		value = defaultValue.value();
	}

	const char* getDisplayName() const { return displayName; }

	operator T()
	{
		return get();
	}

	T& operator=(T const& newValue)
	{
		set(newValue);
		return value.value();
	}

private:
	std::optional<T> value;
	std::optional<T> defaultValue;	// don't remember why defaultValue is optional
	const char* category;
	const char* name;
	const char* displayName;
	bool autoSave;

	void load()
	{
		if (defaultValue == std::nullopt)
		{
			if (Config::doesValueExist(category, name))
			{
				value = Config::getValue<T>(category, name, {});
			}
		}
		else
			value = Config::getValue(category, name, defaultValue.value());
	}
};
