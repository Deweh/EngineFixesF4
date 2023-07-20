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

	namespace Workshop
	{
		bool IsWorkshopItem(const TESObjectREFR* a_refr) {
			using func_t = decltype(&IsWorkshopItem);
			REL::Relocation<func_t> func{ REL::ID(1386903) };
			return func(a_refr);
		}
	}

	struct DynamicNavmesh
	{
		static DynamicNavmesh* GetSingleton() {
			REL::Relocation<DynamicNavmesh**> singleton{ REL::ID(908540) };
			return *singleton;
		}

		void ForceUpdate() {
			using func_t = decltype(&DynamicNavmesh::ForceUpdate);
			REL::Relocation<func_t> func{ REL::ID(854653) };
			return func(this);
		}
	};
}
