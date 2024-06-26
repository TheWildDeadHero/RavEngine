#include "NetworkIdentity.hpp"
#include "App.hpp"

using namespace RavEngine;
bool NetworkIdentity::IsOwner() const
{
	if (GetApp()->networkManager.IsServer()) {
		return Owner == k_HSteamNetConnection_Invalid;
	}
	else {
		return !(Owner == k_HSteamNetConnection_Invalid);
	}
}