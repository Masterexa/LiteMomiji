#pragma once

/* include */
#include "pre.hpp"
#include <array>

struct Input{

	struct XinputState
	{
		XINPUT_STATE		state;
		DirectX::XMFLOAT2	thumb_left;
		DirectX::XMFLOAT2	thumb_right;
		DirectX::XMFLOAT2	triggerLR;
	};

	/* Instance */
		/* Fields */
			std::array<XinputState,4>	m_xinput_states_cur;

		/* Methods */
			void update();

			DirectX::XMVECTOR getLStickVector(uint32_t num) const;
			DirectX::XMVECTOR getRStickVector(uint32_t num) const;
			DirectX::XMVECTOR getTriggerVector(uint32_t num) const;

			bool getXInputState(uint32_t num, XINPUT_STATE* out_state, XINPUT_VIBRATION* out_vibration) const;
};