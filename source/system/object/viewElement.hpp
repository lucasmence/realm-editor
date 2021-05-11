#pragma once

#ifndef VIEWELEMENT_HPP
#define VIEWELEMENT_HPP

class ViewElement
{
	public:
		int priority;
		virtual ~ViewElement();
		virtual bool draw();

};

#endif