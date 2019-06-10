/* delcatation */
#include "Input.hpp"
#include <algorithm>
namespace{
	using namespace DirectX;
}

DirectX::XMVECTOR getStickNormalized(float x, float y, float max_val, float dead_zone)
{
	auto v		= XMVectorSet(x, y, 0.0f, 1.0f);
	auto mag	= XMVectorGetX(XMVector2Length(v));

	v = XMVectorScale(v, 1/mag); // normalize input vector
	auto mag_norm	= 0.0f;

	if(mag>dead_zone)
	{
		mag	= std::min<float>(mag, max_val);
		mag -= dead_zone;

		mag_norm = mag/(max_val-dead_zone);
	}
	else
	{
		mag = 0.f;
	}

	return XMVectorScale(v, mag_norm);
}

void Input::update()
{
	for(size_t i=0; i<m_xinput_states_cur.size(); i++)
	{
		auto& it = m_xinput_states_cur[i];

		auto err = XInputGetState(i, &it.state);
		if(err==ERROR_SUCCESS)
		{
			XMStoreFloat2(&it.thumb_left, getStickNormalized(it.state.Gamepad.sThumbLX, it.state.Gamepad.sThumbLY, 32767.f, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
			XMStoreFloat2(&it.thumb_right, getStickNormalized(it.state.Gamepad.sThumbRX, it.state.Gamepad.sThumbRY, 32767.f, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
			XMStoreFloat2(&it.triggerLR, getStickNormalized(it.state.Gamepad.bLeftTrigger, it.state.Gamepad.bRightTrigger, MAXBYTE, 0));
		}
		else{
			XMStoreFloat2(&it.thumb_left, XMVectorZero());
			XMStoreFloat2(&it.thumb_right, XMVectorZero());
			XMStoreFloat2(&it.triggerLR, XMVectorZero());
		}
	}
}

DirectX::XMVECTOR Input::getLStickVector(uint32_t num) const
{
	assert(num<4);
	return XMLoadFloat2(&m_xinput_states_cur[num].thumb_left);
}

DirectX::XMVECTOR Input::getRStickVector(uint32_t num) const
{
	assert(num<4);
	return XMLoadFloat2(&m_xinput_states_cur[num].thumb_right);
}

DirectX::XMVECTOR Input::getTriggerVector(uint32_t num) const
{
	assert(num<4);
	return XMLoadFloat2(&m_xinput_states_cur[num].triggerLR);
}

bool Input::getXInputState(uint32_t num, XINPUT_STATE* out_state, XINPUT_VIBRATION* out_vibration) const
{
	assert(num<4);
	return false;
}