#include "RE.h"

namespace
{
	class CellAttachDetachListener : public RE::BSTEventSink<RE::TESCellAttachDetachEvent>
	{
	public:
		static CellAttachDetachListener* GetSingleton() {
			static CellAttachDetachListener instance;
			return &instance;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent& a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override
		{
			if (a_event.refr != nullptr && a_event.refr->parentCell != nullptr &&
				a_event.refr->parentCell->cellFlags.any(RE::TESObjectCELL::Flag::kInterior) &&
					RE::Workshop::IsWorkshopItem(a_event.refr.get())) {
				if (auto enctZone = a_event.refr->parentCell->GetEncounterZone(); enctZone != nullptr && enctZone->IsWorkshop()) {
					a_event.refr->UpdateDynamicNavmesh((a_event.refr->IsDeleted() || a_event.refr->IsDisabled()) ? false : a_event.isAttaching);
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	void ReceiveMessage(F4SE::MessagingInterface::Message* msg) {
		if (msg->type == F4SE::MessagingInterface::kGameDataReady) {
			RE::RegisterForCellAttachDetach(CellAttachDetachListener::GetSingleton());
			logger::info("Registered cell attach/detach listener.");
		}
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(spdlog::level::trace);
	log->flush_on(spdlog::level::trace);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	if (a_f4se->IsEditor()) {
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical(FMT_STRING("{} does not support runtime v{}"), Version::PROJECT, ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);
	F4SE::GetMessagingInterface()->RegisterListener(&ReceiveMessage);
	logger::info("Registered message listener.");

	return true;
}
