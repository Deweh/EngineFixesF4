#pragma once

namespace RE
{
	struct TESCellAttachDetachEvent
	{
		RE::NiPointer<RE::TESObjectREFR> refr;
		bool isAttaching;
	};

	void RegisterForCellAttachDetach(RE::BSTEventSink<TESCellAttachDetachEvent>* a_sink) {
		using func_t = decltype(&RegisterForCellAttachDetach);
		REL::Relocation<func_t> func{ REL::ID(409767) };
		return func(a_sink);
	}
}
