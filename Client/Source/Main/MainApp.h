#pragma once

#include "Core/Interfaces/IExecute.h"

class MainApp : public IExecute
{

public :
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;

private :
	void CreateChicken();
};

