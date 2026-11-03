#include "Menu.h"

#include "Editor.h"

#pragma warning(push)
#pragma warning(disable : 4099 5054)
#include "SKSEMenuFramework.h"
#pragma warning(pop)

namespace Menu
{
	namespace
	{
		using namespace ImGuiMCP;

		void Tip(const char* a_text)
		{
			if (IsItemHovered()) {
				SetTooltip("%s", a_text);
			}
		}

		std::uint32_t                  pickedSerial = 0;
		std::vector<Editor::LightSnap> edits;
		bool                           applyAll = false;
		std::uint32_t                  addedSerial = 0;

		struct Pending
		{
			Editor::NewLight l;
			char             buf[128]{};
		};

		bool FlagBox(const char* a_label, Editor::LightSnap& a_s, std::uint32_t a_bit, const char* a_tip)
		{
			bool v = (a_s.flags & a_bit) != 0;
			const bool changed = Checkbox(a_label, &v);
			Tip(a_tip);
			if (changed) {
				a_s.flags = v ? (a_s.flags | a_bit) : (a_s.flags & ~a_bit);
			}
			return changed;
		}

		struct Conditions
		{
			int   when{ 0 };
			float from{ 19.0f };
			float to{ 7.0f };
			char  global[96]{};
		} cond;

		void DrawConditions()
		{
			Combo("When is it lit", &cond.when, "Always on\0Outdoors only\0Indoors only\0At night\0Outdoors, at night\0");
			Tip("Always on: no condition, a permanent light. The others write Light Placer conditions into the saved lights. Applies to the Save buttons in this section and in 'Add a light'.");
			if (cond.when >= 3) {
				SliderFloat("Night from", &cond.from, 0.0f, 24.0f, "%.2f h");
				SliderFloat("Night until", &cond.to, 0.0f, 24.0f, "%.2f h");
			}
			InputText("Only while this global is 1 (optional)", cond.global, sizeof(cond.global));
			Tip("The EditorID of a global variable. The light is lit only while it is 1, so another mod (or a Papyrus script) can switch the light. Leave empty for none.");
		}

		Editor::SaveOptions ConditionOptions()
		{
			Editor::SaveOptions o;
			o.when = cond.when;
			o.nightFrom = cond.from;
			o.nightTo = cond.to;
			o.global = cond.global;
			return o;
		}

		void __stdcall RenderEditor()
		{
			SeparatorText("Pick a light");
			TextWrapped("%s", Editor::StatusLine().c_str());
			if (Button("Pick the light I am looking at")) {
				Editor::RequestPick();
			}
			SameLine();
			if (Button("Pick the object I am looking at (even without a light)")) {
				Editor::RequestPickAny();
			}
			Tip("For adding a light to an object that has none. Picks the object whose origin is closest to where the camera points.");
			Tip("Searches within 2500 units for the light closest to where the camera points. Works on any object whose model carries a light: Light Placer lights, ISL Helper converted lights and placed vanilla lights.");
			SameLine();
			if (Button("Scan Light Placer files")) {
				Editor::RescanJson();
			}
			Tip("Reads Data/LightPlacer/**/*.json so the list below can show which files define the picked object. Press it again after changing files.");
			Checkbox("Edits also change all loaded copies of this object", &applyAll);
			Tip("Within 6000 units of you. Useful to see a change on every lantern of a street at once.");

			const auto t = Editor::GetTarget();
			if (!t.valid) {
				return;
			}
			if (t.serial != pickedSerial) {
				pickedSerial = t.serial;
				edits = t.lights;
			}

			SeparatorText("Object");
			Text("%s", t.baseEdid.empty() ? "(no editor id)" : t.baseEdid.c_str());
			if (!t.name.empty()) {
				SameLine();
				TextDisabled("\"%s\"", t.name.c_str());
			}
			TextDisabled("Reference %08X, base %08X", t.refId, t.baseId);
			TextWrapped("Model: %s", t.model.empty() ? "-" : t.model.c_str());

			SeparatorText("Lights on it");
			if (t.lights.empty()) {
				TextDisabled("This object has no light nodes in its loaded model.");
			}
			for (std::size_t i = 0; i < t.lights.size() && i < edits.size(); ++i) {
				auto& e = edits[i];
				PushID(std::to_string(i).c_str());
				const std::string head = std::format("{}  ({})", e.node.empty() ? "(unnamed node)" : e.node, e.lighEdid.empty() ? "no LIGH" : e.lighEdid);
				if (CollapsingHeader(head.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					bool c = false;
					c |= SliderFloat("Radius", &e.radius, 0.0f, 3000.0f, "%.0f");
					Tip("Reach of the light in game units.");
					c |= SliderFloat("Size", &e.size, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
					Tip("Inverse square size: how large the emitting source is. Small = tight bright hot spot, large = softer.");
					c |= SliderFloat("Cutoff", &e.cutoff, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
					Tip("The light level at which the reach ends. Lower = flatter curve that carries further.");
					c |= SliderFloat("Fade (intensity)", &e.fade, 0.0f, 20.0f, "%.2f");
					c |= ColorEdit3("Colour", e.color);
					c |= FlagBox("Inverse square", e, Editor::Flag::InverseSquare, "Community Shaders inverse square falloff.");
					SameLine();
					c |= FlagBox("Linear", e, Editor::Flag::Linear, "Linear falloff (Light Limit Fix).");
					c |= FlagBox("Shadow", e, Editor::Flag::Shadow, "Casts shadows. Community Shaders limits these to 4 per cell.");
					SameLine();
					c |= FlagBox("Portal strict", e, Editor::Flag::PortalStrict, "Does not light through portals (interior rooms).");
					c |= FlagBox("Ignore scale", e, Editor::Flag::IgnoreScale, "Ignores the object's scale.");
					SameLine();
					c |= FlagBox("No external emittance", e, Editor::Flag::NoExternalEmittance, "Does not follow the time of day colour.");
					if (e.cull != 0) {
						TextDisabled("Hidden by Light Placer (cull bits 0x%02X): conditions, the game or a script. Edits are kept for when it shows again.", e.cull);
					}
					TextDisabled("LIGH record %08X: %s", e.ligh, e.lighIsl ? "carries the inverse square flag (converted by ISL Helper or the author)" : "vanilla falloff");
					if (c) {
						Editor::QueueEdit(e, applyAll);
					}
					if (Button("Revert to the values at pick time")) {
						edits[i] = t.lights[i];
						Editor::QueueRevert(e.node, applyAll);
					}
				}
				PopID();
			}

			SeparatorText("Write it as Light Placer json");
			DrawConditions();
			if (Button("Save to ISLPlacer_Patches.json")) {
				Editor::SaveOptions o;
				o = ConditionOptions();
				Editor::SavePatch(o);
			}
			Tip("Writes all the lights shown above, with the values in the sliders, as one entry for this object's model in Data/LightPlacer/ISLPlacer/ISLPlacer_Patches.json. Light Placer reads it at the next game start. ISL Helper leaves lights alone that a Light Placer file names.");
			SameLine();
			if (Button("Remove my entry for this model")) {
				Editor::RemovePatch();
			}

			SeparatorText("Add a light to this object");
			static std::vector<Pending> added;
			if (t.serial != addedSerial) {
				addedSerial = t.serial;
				added.clear();
			}
			if (Button("Add a light")) {
				Pending p;
				std::snprintf(p.buf, sizeof(p.buf), "%s", "LOS_Candle");
				p.l.ligh = p.buf;
				Editor::LightDefaults(p.l.ligh, p.l);
				added.push_back(p);
			}
			Tip("A light that the object does not have yet. It is created by Light Placer at the next game start (no live preview).");
			for (std::size_t i = 0; i < added.size(); ++i) {
				auto& p = added[i];
				PushID(static_cast<int>(1000 + i));
				Separator();
				if (InputText("Light record (EditorID)", p.buf, sizeof(p.buf))) {
					p.l.ligh = p.buf;
				}
				Tip("The LIGH record the light is based on. It gives the default look; the values below replace it. Use one that exists in your load order, e.g. LOS_Candle.");
				if (Button("Take radius, colour and fade from the record")) {
					if (!Editor::LightDefaults(p.l.ligh, p.l)) {
						p.l.ligh += "  (not found)";
						std::snprintf(p.buf, sizeof(p.buf), "%s", p.l.ligh.c_str());
					}
				}
				bool atNode = p.l.attachKind == 1;
				if (Checkbox("Attach to a node of the model", &atNode)) {
					p.l.attachKind = atNode ? 1 : 2;
				}
				if (atNode) {
					if (BeginCombo("Node", p.l.node.empty() ? "(choose)" : p.l.node.c_str())) {
						for (const auto& n : t.nodes) {
							if (Selectable(n.c_str(), n == p.l.node)) {
								p.l.node = n;
							}
						}
						EndCombo();
					}
				} else {
					DragFloat3("Position (x y z)", p.l.point, 0.5f, -1000.0f, 1000.0f, "%.1f");
					Tip("Offset from the origin of the model, in game units: x right, y forward, z up.");
				}
				SliderFloat("Radius", &p.l.radius, 0.0f, 3000.0f, "%.0f");
				SliderFloat("Size", &p.l.size, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
				SliderFloat("Cutoff", &p.l.cutoff, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
				SliderFloat("Fade (intensity)", &p.l.fade, 0.0f, 20.0f, "%.2f");
				ColorEdit3("Colour", p.l.color);
				const auto bit = [&](const char* a_label, std::uint32_t a_bit) {
					bool v = (p.l.flags & a_bit) != 0;
					if (Checkbox(a_label, &v)) {
						p.l.flags = v ? (p.l.flags | a_bit) : (p.l.flags & ~a_bit);
					}
				};
				bit("Inverse square", Editor::Flag::InverseSquare);
				SameLine();
				bit("Linear", Editor::Flag::Linear);
				bit("Shadow", Editor::Flag::Shadow);
				SameLine();
				bit("Portal strict", Editor::Flag::PortalStrict);
				bit("Ignore scale", Editor::Flag::IgnoreScale);
				SameLine();
				bit("No external emittance", Editor::Flag::NoExternalEmittance);
				if (Button("Remove this light")) {
					added.erase(added.begin() + static_cast<std::ptrdiff_t>(i));
					PopID();
					break;
				}
				PopID();
			}
			if (!added.empty() && Button("Save the new lights")) {
				std::vector<Editor::NewLight> list;
				for (const auto& p : added) {
					list.push_back(p.l);
				}
				Editor::SaveOptions o;
				o = ConditionOptions();
				Editor::SaveNewLights(list, o);
			}

			if (const auto ps = Editor::PatchStatus(); !ps.empty()) {
				TextWrapped("%s", ps.c_str());
			}

			SeparatorText("Light Placer files that define it");
			const auto m = Editor::FindMatches();
			if (m.entriesScanned == 0) {
				TextDisabled("Press 'Scan Light Placer files' first.");
			} else {
				TextDisabled("%d entries in %d files scanned", m.entriesScanned, m.filesScanned);
				if (m.list.empty()) {
					TextWrapped("No Light Placer entry names this model or object. Its lights come from somewhere else (ISL Helper, a placed light or the model itself).");
				}
				for (const auto& x : m.list) {
					std::string nodes;
					for (const auto& n : x.nodes) {
						nodes += (nodes.empty() ? "" : ", ") + n;
					}
					std::string lights;
					for (const auto& n : x.lights) {
						lights += (lights.empty() ? "" : ", ") + n;
					}
					BulletText("%s  #%d  (by %s)%s", x.file.c_str(), x.entry, x.by.c_str(), x.conditions ? "  [conditions]" : "");
					if (x.file != Editor::kPatchFile) {
						PushID((x.file + "#" + std::to_string(x.entry)).c_str());
						SameLine();
						if (SmallButton("Disable this file")) {
							Editor::DisableFile(x.file);
						}
						Tip("Renames the whole file to .json.hidden so Light Placer stops reading it (takes effect at the next game load). This switches off every light in that file, not only this object. Nothing else is touched; you can re-enable it below.");
						PopID();
					}
					TextDisabled("    nodes: %s | lights: %s", nodes.empty() ? "-" : nodes.c_str(), lights.empty() ? "-" : lights.c_str());
				}
				for (const auto& c : m.conflicts) {
					TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", c.c_str());
				}
				if (!m.conflicts.empty()) {
					TextDisabled("Each entry adds its own bulb. Disable the other file (button above) or remove our entry.");
				}
			}

			const auto hidden = Editor::HiddenFiles();
			if (!hidden.empty()) {
				SeparatorText("Files I disabled");
				for (const auto& f : hidden) {
					PushID(f.c_str());
					TextWrapped("%s", f.c_str());
					SameLine();
					if (SmallButton("Re-enable")) {
						Editor::EnableFile(f);
					}
					PopID();
				}
			}
			const auto changed = Editor::ChangedFiles();
			if (!changed.empty()) {
				SeparatorText("Files I changed");
				for (const auto& f : changed) {
					PushID(f.c_str());
					TextWrapped("%s", f.c_str());
					SameLine();
					if (SmallButton("Put back")) {
						Editor::RestoreFile(f);
					}
					PopID();
				}
			}
		}

	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::info("SKSE Menu Framework is not installed: the light editor has no menu");
			return;
		}
		SKSEMenuFramework::SetSection("ISL Placer");
		SKSEMenuFramework::AddSectionItem("Light editor", RenderEditor);
		logger::info("Registered the light editor page with the SKSE Menu Framework");
	}
}
