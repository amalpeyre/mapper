/*
 *    Copyright 2012, 2013 Thomas Schöps
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "map_color.h"

#include <QDebug>

#include "../map.h"


MapColor::MapColor()
: name("?"),
  priority(Undefined),
  opacity(1.0f),
  spot_color_method(MapColor::UndefinedMethod),
  cmyk_color_method(MapColor::CustomColor),
  rgb_color_method(MapColor::CmykColor),
  flags(0),
  spot_color_name("")
{
	Q_ASSERT(isBlack());
}

MapColor::MapColor(int priority)
: name("?"),
  priority(priority),
  opacity(1.0f),
  spot_color_method(MapColor::UndefinedMethod),
  cmyk_color_method(MapColor::CustomColor),
  rgb_color_method(MapColor::CmykColor),
  flags(0),
  spot_color_name("")
{
	Q_ASSERT(isBlack());
	
	switch (priority)
	{
		case CoveringWhite:
			setCmyk(QColor(Qt::white));
			Q_ASSERT(isWhite());
			opacity = 1000.0f;	// HACK: (almost) always opaque, even if multiplied by opacity factors
			break;
		case CoveringRed:
			setRgb(QColor(Qt::red));
			setCmykFromRgb();
			opacity = 1000.0f;	// HACK: (almost) always opaque, even if multiplied by opacity factors
			break;
		case Undefined:
			setCmyk(QColor(Qt::darkGray));
			opacity = 1.0f;
			break;
	}	
}

MapColor::MapColor(const QString& name, int priority)
: name(name),
  priority(priority),
  opacity(1.0),
  spot_color_method(MapColor::UndefinedMethod),
  cmyk_color_method(MapColor::CustomColor),
  rgb_color_method(MapColor::CmykColor),
  flags(0),
  spot_color_name("")
{
	Q_ASSERT(isBlack());
}


MapColor* MapColor::duplicate() const
{
	MapColor* copy = new MapColor(name, priority);
	copy->cmyk = cmyk;
	copy->rgb = rgb;
	copy->opacity = opacity;
	copy->q_color = q_color;
	copy->spot_color_method = spot_color_method;
	copy->cmyk_color_method = cmyk_color_method;
	copy->rgb_color_method  = rgb_color_method;
	copy->spot_color_name = spot_color_name;
	copy->components = components;
	return copy;
}

bool MapColor::isBlack() const
{
	Q_ASSERT(rgb.isBlack() == cmyk.isBlack());
	return rgb.isBlack();
}

bool MapColor::isWhite() const
{
	Q_ASSERT(rgb.isWhite() == cmyk.isWhite());
	return rgb.isWhite();
}


bool MapColor::equals(const MapColor& other, bool compare_priority) const
{
	// FIXME: MapColor::equals may be incomplete regarding new features
	return (name.compare(other.name, Qt::CaseInsensitive) == 0) &&
		   (!compare_priority || (priority == other.priority)) && 
		   (qAbs(cmyk.c - other.cmyk.c) < 1e-03) && (qAbs(cmyk.m - other.cmyk.m) < 1e-03) &&
		   (qAbs(cmyk.y - other.cmyk.y) < 1e-03) && (qAbs(cmyk.k - other.cmyk.k) < 1e-03) &&
		   (qAbs(opacity - other.opacity) < 1e-03);
}


void MapColor::setSpotColorName(const QString& spot_color_name) 
{ 
	spot_color_method = MapColor::SpotColor;
	this->spot_color_name = spot_color_name;
	components.clear();
	
	if (cmyk_color_method == MapColor::SpotColor)
	{
		// CMYK cannot be determined from a pure spot color.
		cmyk_color_method = MapColor::CustomColor;
		q_color = cmyk;
	}
	
	if (rgb_color_method == MapColor::SpotColor)
	{
		// RGB cannot be determined from a pure spot color.
		rgb_color_method = MapColor::CustomColor;
	}
}

void MapColor::setSpotColorComposition(const SpotColorComponents& components)
{
	this->components = components;
	spot_color_name = "";
	
	if (components.size() > 0)
	{
		spot_color_method = MapColor::CustomColor;
		
		// Determine a display name.
		for (SpotColorComponents::const_iterator it = components.begin(); ; )
		{
			spot_color_name += QString("%1 %2%").
			  arg(it->spot_color->getSpotColorName()).
			  arg(QString::number(it->factor * 100) /* % */
			);
			it++;
			if (it == components.end())
				break;
			spot_color_name += ", ";
		}
			  
		
		if (cmyk_color_method == MapColor::SpotColor)
		{
			// Update CMYK from spot colors
			cmyk = cmykFromSpotColors();
			q_color = cmyk;
			
			if (rgb_color_method == MapColor::CmykColor)
				// Update RGB from CMYK
				rgb = MapColorRgb(q_color);
		}
		
		if (rgb_color_method == MapColor::SpotColor)
		{
			// Update RGB from spot colors
			rgb = rgbFromSpotColors();
			
			if (cmyk_color_method == MapColor::RgbColor)
			{
				// Update CMYK from RGB
				q_color = rgb;
				cmyk = MapColorCmyk(q_color);
			}
		}
	}
	else if (spot_color_method == MapColor::CustomColor)
	{
		// An empty composition is not a valid custom spot color
		spot_color_method = MapColor::UndefinedMethod;
		
		if (cmyk_color_method == MapColor::SpotColor)
			// Cannot determine CMYK from empty composition
			cmyk_color_method = MapColor::CustomColor;
		
		if (rgb_color_method == MapColor::SpotColor)
			// Cannot determine RGB from empty composition
			rgb_color_method = MapColor::CustomColor;
	}
	else
		spot_color_method = MapColor::UndefinedMethod;
	
	Q_ASSERT(this->components.size() == 0 || spot_color_method == MapColor::CustomColor);
	Q_ASSERT(this->components.size() >  0 || spot_color_method != MapColor::CustomColor);
	Q_ASSERT(this->components.size() >  0 || cmyk_color_method != MapColor::SpotColor);
}

void MapColor::setKnockout(bool flag)
{
	if (flag)
	{
		if (!getKnockout())
			flags += MapColor::Knockout;
	}
	else if (getKnockout())
		flags -= MapColor::Knockout;
	
	Q_ASSERT(getKnockout() == flag);
}

bool MapColor::getKnockout() const
{
	return (flags & MapColor::Knockout) > 0;
}


void MapColor::setCmyk(const MapColorCmyk& cmyk)
{
	cmyk_color_method = MapColor::CustomColor;
	this->cmyk = cmyk;
	q_color = cmyk;
	
	if (rgb_color_method == MapColor::CmykColor)
		// Update RGB from CMYK
		rgb = MapColorRgb(q_color);
}

void MapColor::setCmykFromSpotColors()
{
	if (spot_color_method == MapColor::CustomColor)
	{
		if (components.size() > 0)
		{
			cmyk_color_method = MapColor::SpotColor;
			cmyk = cmykFromSpotColors();
			q_color = cmyk;
			if (rgb_color_method == MapColor::CmykColor)
				rgb = MapColorRgb(q_color);
		}
		else if (components.size() == 0)
		{
			// Switch from invalid to valid state
			spot_color_method = MapColor::UndefinedMethod;
			if (cmyk_color_method == MapColor::SpotColor)
				cmyk_color_method = MapColor::CustomColor;
			if (rgb_color_method == MapColor::SpotColor)
				rgb_color_method = MapColor::CustomColor;
		}
	}
}

void MapColor::setCmykFromRgb()
{
	if (rgb_color_method == MapColor::CmykColor)
		rgb_color_method = MapColor::CustomColor;
	
	cmyk_color_method = MapColor::RgbColor;
	q_color = rgb;
	cmyk = MapColorCmyk(q_color);
}


void MapColor::setRgb(const MapColorRgb& rgb)
{
	rgb_color_method = MapColor::CustomColor;
	this->rgb = rgb;
	
	if (cmyk_color_method == MapColor::RgbColor)
	{
		q_color = rgb;
		cmyk = q_color;
	}
}

void MapColor::setRgbFromSpotColors()
{
	if (spot_color_method == MapColor::CustomColor)
	{
		if (components.size() > 0)
		{
			rgb_color_method = MapColor::SpotColor;
			rgb = rgbFromSpotColors();
			if (cmyk_color_method == MapColor::RgbColor)
			{
				q_color = rgb;
				cmyk = MapColorCmyk(q_color);
			}
		}
		else if (components.size() == 0)
		{
			// Switch from invalid to valid state
			spot_color_method = MapColor::UndefinedMethod;
			if (cmyk_color_method == MapColor::SpotColor)
				cmyk_color_method = MapColor::CustomColor;
			if (rgb_color_method == MapColor::SpotColor)
				rgb_color_method = MapColor::CustomColor;
		}
	}
}

void MapColor::setRgbFromCmyk()
{
	if (cmyk_color_method == MapColor::RgbColor)
		cmyk_color_method = MapColor::CustomColor;
	
	rgb_color_method = MapColor::CmykColor;
	q_color = cmyk;
	rgb = MapColorRgb(q_color);
}

MapColorCmyk MapColor::cmykFromSpotColors() const
{
	Q_ASSERT(components.size() > 0);
	
	MapColorCmyk cmyk(QColor(Qt::white));
	Q_ASSERT(cmyk.isWhite());
	Q_FOREACH(SpotColorComponent component, components)
	{
		const MapColorCmyk& other = component.spot_color->cmyk;
		cmyk.c = cmyk.c + component.factor * other.c * (1.0f - cmyk.c);
		cmyk.m = cmyk.m + component.factor * other.m * (1.0f - cmyk.m);
		cmyk.y = cmyk.y + component.factor * other.y * (1.0f - cmyk.y);
		cmyk.k = cmyk.k + component.factor * other.k * (1.0f - cmyk.k);
	}
	return cmyk;
}

MapColorRgb MapColor::rgbFromSpotColors() const
{
	Q_ASSERT(components.size() > 0);
	
	MapColorRgb rgb = QColor(Qt::white);
	Q_ASSERT(rgb.isWhite());
	Q_FOREACH(SpotColorComponent component, components)
	{
		const MapColorRgb& other = component.spot_color->rgb;
		rgb.r = rgb.r - component.factor * (1.0f - other.r) * rgb.r;
		rgb.g = rgb.g - component.factor * (1.0f - other.g) * rgb.g;
		rgb.b = rgb.b - component.factor * (1.0f - other.b) * rgb.b;
	}
	return rgb;
}