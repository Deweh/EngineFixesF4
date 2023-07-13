#include "RE/Bethesda/FormComponents.h"

#include "RE/Bethesda/BSTHashMap.h"
#include "RE/Bethesda/FormUtil.h"
#include "RE/Bethesda/TESBoundAnimObjects.h"
#include "RE/Bethesda/TESForms.h"

namespace RE
{
	namespace detail
	{
		BGSKeyword* BGSKeywordGetTypedKeywordByIndex(KeywordType a_type, std::uint16_t a_index)
		{
			return BGSKeyword::GetTypedKeywordByIndex(a_type, a_index);
		}
		uint16_t BGSKeywordGetIndexForTypedKeyword(BGSKeyword* a_keyword, KeywordType a_type)
		{
			return BGSKeyword::GetIndexForTypedKeyword(a_keyword, a_type);
		}
	}

    bool BGSKeywordForm::ContainsKeywordString(std::string_view a_editorID) const
    {
		if (keywords) {
			for (std::uint32_t idx = 0; idx < numKeywords; ++idx) {
				if (keywords[idx] && keywords[idx]->formEditorID.contains(a_editorID)) {
					return true;
				}
			}
		}

		return false;
    }

    bool BGSKeywordForm::HasKeywordID(std::uint32_t a_formID) const
    {
		if (keywords) {
			for (std::uint32_t idx = 0; idx < numKeywords; ++idx) {
				if (keywords[idx] && keywords[idx]->formID == a_formID) {
					return true;
				}
			}
		}

		return false;
    }

    bool BGSKeywordForm::HasKeywordString(std::string_view a_editorID) const
    {
		if (keywords) {
			for (std::uint32_t idx = 0; idx < numKeywords; ++idx) {
				if (keywords[idx] && keywords[idx]->formEditorID == a_editorID) {
					return true;
				}
			}
		}

		return false;
    }

	std::string_view TESFullName::GetFullName(const TESForm& a_form, bool a_strict)
	{
		if (const auto fullName = a_form.As<TESFullName>(); fullName) {
			const auto name = fullName->GetFullName();
			return name ? name : ""sv;
		} else {
			if (a_strict) {
				switch (a_form.GetFormType()) {
				case ENUM_FORM_ID::kKYWD:  // BGSKeyword
				case ENUM_FORM_ID::kLCRT:  // BGSLocationRefType
				case ENUM_FORM_ID::kAACT:  // BGSAction
				case ENUM_FORM_ID::kLIGH:  // TESObjectLIGH
				case ENUM_FORM_ID::kSTAT:  // TESObjectSTAT
				case ENUM_FORM_ID::kSCOL:  // BGSStaticCollection
				case ENUM_FORM_ID::kMSTT:  // BGSMovableStatic
				case ENUM_FORM_ID::kFLST:  // BGSListForm
					break;
				default:
					return ""sv;
				}
			}

			const auto& map = GetSparseFullNameMap();
			const auto it = map.find(std::addressof(a_form));
			return it != map.end() ? it->second : ""sv;
		}
	}

	void TESFullName::SetFullName(TESForm& a_form, std::string_view a_fullName)
	{
		const auto full = a_form.As<TESFullName>();
		if (full) {
			full->fullName = a_fullName;
			if (const auto actor = a_form.As<TESActorBase>(); actor) {
				actor->AddChange(CHANGE_TYPES::kActorBaseFullName);
			} else if (const auto cell = a_form.As<TESObjectCELL>(); cell) {
				cell->AddChange(CHANGE_TYPES::kCellFullname);
			} else {
				a_form.AddChange(CHANGE_TYPES::kBaseObjectFullName);
			}
		}
	}

	void TESValueForm::SetFormValue(TESForm& a_form, std::int32_t a_value)
	{
		const auto val = a_form.As<TESValueForm>();
		if (val) {
			val->value = a_value;
			a_form.AddChange(CHANGE_TYPES::kBaseObjectValue);
		}
	}
}
