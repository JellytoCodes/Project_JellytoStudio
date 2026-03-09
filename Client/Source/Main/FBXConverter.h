#pragma once

#include "Core/Interfaces/IExecute.h"

class FBXConverter : public IExecute
{

public:
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
};

