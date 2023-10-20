//Uncomment this to disable multithreading with the Parallel Patterns Library.
//#define NO_PPL

#include "RE.h"
#include <Windows.h>

#ifndef NO_PPL
#include <ppl.h>
#endif

namespace
{
	inline static bool PerfCounterFreqAcquired = false;
	inline static double PerfCounterFreq = 0.0;

	static void GetPerfCounterFreq()
	{
		if (!PerfCounterFreqAcquired) {
			LARGE_INTEGER li;
			if (QueryPerformanceFrequency(&li)) {
				PerfCounterFreq = double(li.QuadPart) / 1000.0;
				PerfCounterFreqAcquired = true;
			} else {
				logger::warn("QueryPerformanceFrequency failed!");
			}
		}
	}

	static int64_t StartPerfCounter()
	{
		GetPerfCounterFreq();
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return li.QuadPart;
	}

	static double GetPerfCounterMS(int64_t& counter)
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		auto result = (double(li.QuadPart - (counter)) / PerfCounterFreq);
		counter = li.QuadPart;
		return result;
	}

	void DoUpdateNavmesh(RE::TESObjectREFR* refr, bool attaching) {
		if (refr->parentCell->cellFlags.any(RE::TESObjectCELL::Flag::kInterior) &&
			RE::Workshop::IsWorkshopItem(refr)) {
			if (auto enctZone = refr->parentCell->GetEncounterZone(); enctZone != nullptr && enctZone->IsWorkshop()) {
				refr->UpdateDynamicNavmesh((refr->IsDeleted() || refr->IsDisabled()) ? false : attaching);
			}
		}
	}

	class CellAttachDetachListener : public RE::BSTEventSink<RE::TESCellAttachDetachEvent>
	{
	public:
		inline static std::atomic<bool> updateTaskQueued = false;

		static CellAttachDetachListener* GetSingleton() {
			static CellAttachDetachListener instance;
			return &instance;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent& a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override
		{
			if (a_event.refr != nullptr && a_event.refr->parentCell != nullptr) {
				DoUpdateNavmesh(a_event.refr.get(), a_event.isAttaching);
				if (!updateTaskQueued) {
					updateTaskQueued = true;
					F4SE::GetTaskInterface()->AddTask([]() {
						//auto perfTimer = StartPerfCounter();
						RE::DynamicNavmesh::GetSingleton()->ForceUpdate();
						//always ends up being <1ms
						//logger::info("Finished detach/attach update in {:.0f}ms", GetPerfCounterMS(perfTimer));
						updateTaskQueued = false;
					});
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	void ReceiveMessage(F4SE::MessagingInterface::Message* msg) {
		switch (msg->type) {
			case F4SE::MessagingInterface::kGameDataReady:
			{
				RE::RegisterForCellAttachDetach(CellAttachDetachListener::GetSingleton());
				logger::info("Registered cell attach/detach listener.");
				break;
			}
			case F4SE::MessagingInterface::kPostLoadGame:
			{
				auto perfTimer = StartPerfCounter();

				auto playerCell = RE::PlayerCharacter::GetSingleton()->parentCell;
				//logger::info("Player cell is: {:08X}", playerCell->formID);

				const auto& [map, lock] = RE::TESForm::GetAllForms();
				RE::BSAutoReadLock l{ lock };

#ifdef NO_PPL
				for (const RE::BSTTuple<const uint32_t, RE::TESForm*>& ele : *map) {
#else
				concurrency::parallel_for_each(map->begin(), map->end(), [&](RE::BSTTuple<const uint32_t, RE::TESForm*> ele) {
#endif
					RE::TESObjectCELL* cell = ele.second->As<RE::TESObjectCELL>();

					if (cell) {
						std::vector<RE::NiPointer<RE::TESObjectREFR>> refs;
						cell->spinLock.lock();
						auto& references = cell->references;

						for (uint32_t i = 0; i < references.size(); i++) {
							refs.push_back(references[i]);
						}
						cell->spinLock.unlock();

						for (auto& ref : refs) {
							if (ref != nullptr && ref->parentCell != nullptr) {
								DoUpdateNavmesh(ref.get(), ref->parentCell == playerCell);
							}
						}
					}
				}
#ifndef NO_PPL
				);
#endif


				RE::DynamicNavmesh::GetSingleton()->ForceUpdate();

				logger::info("Finished load-time navmesh updates in {:.0f}ms", GetPerfCounterMS(perfTimer));
				break;
			}
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
