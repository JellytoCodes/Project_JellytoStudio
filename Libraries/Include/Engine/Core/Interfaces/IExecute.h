#pragma once

#include "App/DetailWindow.h"
#include "App/ItemWindow.h"

class IExecute
{

public:
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;

	std::shared_ptr<Scene> GetScene() const		{ return _scene; }

protected :
	std::shared_ptr<ItemWindow>					_itemWindow;
	std::shared_ptr<DetailWindow>				_detailWindow;

	std::shared_ptr<Scene>      _scene;
};
