#pragma once

#include <entt/entt.hpp>

#include "component.hpp"

namespace kawe {

class EntityHierarchy {
public:
    EntityHierarchy(std::optional<entt::entity> &s) : selected{s}, root{entt::null, {}} {}

    std::optional<entt::entity> &selected;

    auto draw(entt::registry &world)
    {
        if (!ImGui::Begin("KAWE: Entity Hierarchy")) return ImGui::End();

        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
        if (ImGui::Selectable("none", !selected.has_value())) { selected = {}; }

        root = {entt::null, {}};

        std::list<entt::entity> pending;
        world.each([&pending](const auto &e) { pending.push_back(e); });

        while (!pending.empty()) {
            for (auto it = pending.begin(); it != pending.end();) {
                // get the node parent of the current entity
                const auto node = [this, &world](const auto &entity) -> Node * {
                    // if no parent, the parent is root
                    if (const auto parent = world.try_get<Parent>(entity); parent == nullptr) {
                        return &root;
                    } else {
                        if (parent->component == entt::null) { return &root; }

                        // find recursively the right node
                        std::function<Node *(Node &)> recurse_find_parent_node;
                        recurse_find_parent_node = [&recurse_find_parent_node, &parent](Node &entry) -> Node * {
                            const auto parent_node =
                                std::find_if(begin(entry.children), end(entry.children), [&parent](auto &i) {
                                    return i.e == parent->component;
                                });
                            if (parent_node != std::end(entry.children)) {
                                return &(*parent_node);
                            } else {
                                for (auto &i : entry.children) {
                                    if (const auto found = recurse_find_parent_node(i); found != nullptr) {
                                        return found;
                                    }
                                }
                                // not found
                                return nullptr;
                            }
                        };
                        return recurse_find_parent_node(root);
                    }
                }(*it);
                // if found, add it to the graph and remove from the pending queue
                if (node != nullptr) {
                    node->children.emplace_back(*it, std::vector<Node>{});
                    it = pending.erase(it);
                } else {
                    it++;
                }
            }
        }

        std::function<void(const std::vector<Node> &)> recurse_convert_to_imgui;
        recurse_convert_to_imgui = [this, &world, &recurse_convert_to_imgui](const std::vector<Node> &entry) {
            constexpr ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
                                                      | ImGuiTreeNodeFlags_OpenOnDoubleClick
                                                      | ImGuiTreeNodeFlags_SpanAvailWidth;

            for (const auto &i : entry) {
                const auto &name = world.get_or_emplace<Name>(i.e, fmt::format("<kawe:anonymous#{}>", i.e));

                auto node_flags = base_flags;
                if (selected.has_value() && selected.value() == i.e) {
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                }

                if (i.children.empty()) {
                    node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                    ImGui::TreeNodeEx(
                        reinterpret_cast<void *>(static_cast<intptr_t>(i.e)),
                        node_flags,
                        "%s",
                        name.component.data());
                    if (ImGui::IsItemClicked()) { selected = i.e; }
                } else {
                    const auto node_open = ImGui::TreeNodeEx(
                        reinterpret_cast<void *>(static_cast<intptr_t>(i.e)),
                        node_flags,
                        "%s",
                        name.component.data());
                    if (ImGui::IsItemClicked()) { selected = i.e; }
                    if (node_open) {
                        recurse_convert_to_imgui(i.children);
                        ImGui::TreePop();
                    }
                }
            }
        };
        recurse_convert_to_imgui(root.children);

        ImGui::EndChild();
        if (ImGui::Button("Create")) {
            const auto created = world.create();
            spdlog::warn("Creating entity {}", created);
            selected = created;
        }

        ImGui::End();
    }

private:
    struct Node {
        // Node *parent;
        entt::entity e;
        std::vector<Node> children;
    };

    Node root;

}; // namespace kawe

} // namespace kawe
