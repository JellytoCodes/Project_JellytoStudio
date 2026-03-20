#pragma once

#include "App/DetailWindow.h"
#include "App/ItemWindow.h"

class IExecute
{

public:
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;

	void SetItemWindow(ItemWindow* w)			{ _itemWindow = w; }
	void SetDetailWindow(DetailWindow* w)		{ _detailWindow = w; }

	std::shared_ptr<Scene> GetScene() const		{ return _scene; }

protected :
	ItemWindow*					_itemWindow = nullptr;
	DetailWindow*				_detailWindow = nullptr;

	std::shared_ptr<Scene>      _scene;
};
