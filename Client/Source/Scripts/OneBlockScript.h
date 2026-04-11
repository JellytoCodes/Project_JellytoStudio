#pragma once
#include "Entity/Components/MonoBehaviour.h"

class Model;
class ModelRenderer;

class OneBlockScript : public MonoBehaviour
{
public:
	OneBlockScript();
	virtual ~OneBlockScript() = default;

	virtual void Awake()      override {}
	virtual void Start()      override;
	virtual void Update()     override;
	virtual void LateUpdate() override {}
	virtual void OnDestroy()  override {}

	void SetCharacterEntity(Entity* ch) { _character = ch; }

	int32 GetTotalBreaks()  const { return _totalBreaks; }
	int32 GetCurrentPhase() const { return _currentPhase; }
	bool  IsBroken()        const { return _isBroken; }

private:
	struct PhaseData
	{
		std::wstring modelName;
		std::wstring dropModel;
		std::wstring phaseName;
		int32        breaksToNext;
	};
	static const std::vector<PhaseData>& GetPhaseTable();

	void TryMine();
	void Mine();
	void Respawn();
	void UpdatePhase();
	void ApplyPhaseModel(const std::wstring& modelName);
	void SpawnDropBlock(const std::wstring& modelName);
	bool IsCharacterNearby();

	Entity* _character = nullptr;

	int32 _totalBreaks  = 0;
	int32 _currentPhase = 0;
	bool  _isBroken     = false;
	float _respawnTimer = 0.f;

	static constexpr float RESPAWN_DELAY = 2.5f;
	static constexpr float MINE_RANGE    = 3.0f;

	std::vector<std::shared_ptr<Model>> _phaseModels;
	std::vector<Model*> _dropModelPtrs;

	static const Vec3 s_dropOffsets[4];
};
