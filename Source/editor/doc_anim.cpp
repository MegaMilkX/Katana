#include "doc_anim.hpp"

#include "../common/util/imgui_helpers.hpp"


static void extractYRotation(const gfxm::quat q, gfxm::quat& out_q, gfxm::mat3& out_m3) {
    out_m3 = gfxm::to_mat3(q);
    out_m3[1] = gfxm::vec3(0, 1, 0);
    out_m3[2].y = 0;
    out_m3[2] = gfxm::normalize(out_m3[2]);
    out_m3[0] = gfxm::cross(out_m3[1], out_m3[2]);
    out_q = gfxm::to_quat(out_m3);
}

DocAnim::DocAnim() {
    hLight = world.createEntity();
}

static gfxm::mat4 getParentTransform(Skeleton* skeleton, const std::vector<AnimSample>& samples, int32_t bone_idx) {
    auto bone = skeleton->getBone(bone_idx);
    std::vector<int32_t> chain;
    while(bone.parent >= 0) {
        chain.push_back(bone.parent);
        bone = skeleton->getBone(bone.parent);
    }

    gfxm::mat4 m(1.0f);
    for(int i = chain.size() - 1; i >= 0; --i) {
        gfxm::mat4 lcl = 
            gfxm::translate(gfxm::mat4(1.0f), samples[chain[i]].t)
            * gfxm::to_mat4(samples[chain[i]].r)
            * gfxm::scale(gfxm::mat4(1.0f), samples[chain[i]].s);
        m = m * lcl;
    }
    return m;
}

void DocAnim::onGui(Editor* ed, float dt) {
    std::vector<ecsEntityHandle> tgt_nodes;
    if(ref_skel) {
        auto& skel = ref_skel;

        float prev_cursor = cursor;
        if(playing) {
            cursor += dt * (_resource->fps) * playback_speed;
            if(cursor > _resource->length) {
                cursor -= _resource->length;
            }
        }

        std::vector<int32_t> mapping = _resource->getMapping(skel.get());
        int root_motion_anim_node_id = _resource->getNodeIndex(_resource->root_motion_node_name);
        int root_motion_bone_id = -1;
        if(root_motion_anim_node_id >= 0) {
            root_motion_bone_id = mapping[root_motion_anim_node_id];
        }
        
        AnimSampleBuffer sample_buf(skel.get());
        //std::vector<AnimSample> zero_pose = skel->makePoseArray();
        //_resource->sample_remapped(pose, .0f, mapping);
        _resource->sample_remapped(sample_buf, prev_cursor, cursor, skel.get(), mapping);

/*
        // ROOT MOTION
        ktNode* root_motion_node = scn.findObject(_resource->root_motion_node_name);

        gfxm::vec3 t0;
        if (root_motion_bone_id >= 0) {
            t0 = root_motion_node->getTransform()->getWorldPosition();
        }

        AnimSample rm_zero_sample;
        AnimSample rm_end_sample;
        AnimSample rm_sample_a;
        AnimSample rm_sample_b;
        
        gfxm::quat delta_rotation;
        gfxm::quat zero_delta_rotation;
        gfxm::quat y_rotation_lcl;
        
        if(root_motion_bone_id >= 0 && root_motion_node) {
            ktNode* root_motion_node_parent = root_motion_node->getParent();
            _resource->sample_one(root_motion_anim_node_id, .0f, rm_zero_sample);
            _resource->sample_one(root_motion_anim_node_id, _resource->length, rm_end_sample);
            _resource->sample_one(root_motion_anim_node_id, prev_cursor, rm_sample_a);
            _resource->sample_one(root_motion_anim_node_id, cursor, rm_sample_b);

            gfxm::mat4 m_rm_parent_world = root_motion_node_parent->getTransform()->getWorldTransform();
            gfxm::vec3 t0;
            gfxm::vec3 t1;
            t0 = (m_rm_parent_world) * gfxm::vec4(rm_sample_a.t, 1.0f);
            t1 = (m_rm_parent_world) * gfxm::vec4(rm_sample_b.t, 1.0f);
            gfxm::quat q_rm_parent_world = root_motion_node_parent->getTransform()->getWorldRotation();
            gfxm::quat q0 = (q_rm_parent_world) * rm_sample_a.r;
            gfxm::quat q1 = (q_rm_parent_world) * rm_sample_b.r;

            gfxm::mat3 rm0, rm1, rm_zero, rm_end;
            gfxm::quat q_zero = q_rm_parent_world * rm_zero_sample.r;
            gfxm::quat q_end = q_rm_parent_world * rm_end_sample.r;
            extractYRotation(q0, q0, rm0);
            extractYRotation(q1, q1, rm1);
            extractYRotation(q_zero, q_zero, rm_zero);
            extractYRotation(q_end, q_end, rm_end);

            zero_delta_rotation = q1 * gfxm::inverse(q_zero);

            y_rotation_lcl = q1 * gfxm::inverse(root_motion_node_parent->getTransform()->getWorldRotation());

            gfxm::vec3 dbg_q_forward_0 = gfxm::normalize(gfxm::to_mat4(rm0) * gfxm::vec4(root->getTransform()->back(), 0.0f)) * 2;
            gfxm::vec3 dbg_q_forward_1 = gfxm::normalize(gfxm::to_mat4(rm1) * gfxm::vec4(root->getTransform()->back(), 0.0f)) * 2;
            

            gvp.getDebugDraw().point(t0, gfxm::vec3(1,0,0));
            gvp.getDebugDraw().point(t1, gfxm::vec3(1,1,0));

            gfxm::vec3 delta_translation;
            if(cursor < prev_cursor) {
                delta_translation = 
                    (gfxm::vec3(m_rm_parent_world * gfxm::vec4(rm_end_sample.t, 1.0f)) - t0) +
                    (t1 - gfxm::vec3(m_rm_parent_world * gfxm::vec4(rm_zero_sample.t, 1.0f)));

                delta_rotation = 
                    (q1 * gfxm::inverse(q_zero)) *
                    (q_end * gfxm::inverse(q0));
            } else {
                delta_translation = t1 - t0;
                delta_rotation = q1 * gfxm::inverse(q0);
            }

            gfxm::vec3 dbg_delta_fwd = gfxm::normalize(gfxm::to_mat4(gfxm::to_mat3(delta_rotation)) * gfxm::vec4(root->getTransform()->back(), 0.0f)) * 2;
            
            //gfxm::mat4 mat = gfxm::to_mat4(root->getTransform()->getWorldRotation());
            delta_translation = gfxm::to_mat4(gfxm::inverse(zero_delta_rotation)) * gfxm::vec4(delta_translation, .0f);
            delta_translation.y = .0f;
            root->getTransform()->translate(delta_translation);
            root->getTransform()->rotate(delta_rotation);

            gvp.getDebugDraw().point(root->getTransform()->getWorldPosition(), gfxm::vec3(0,1,0));
            gfxm::vec3 origin = root->getTransform()->getWorldPosition();
            gvp.getDebugDraw().line(origin, origin + gfxm::normalize(root->getTransform()->back()) * 2.0f, gfxm::vec3(0,1,0));
            gvp.getDebugDraw().line(origin, origin + dbg_q_forward_0, gfxm::vec3(1, 0.5, 1));
            gvp.getDebugDraw().line(origin, origin + dbg_q_forward_1, gfxm::vec3(1, 0, 1));
            //gvp.getDebugDraw().line(origin, origin + dbg_delta_fwd, gfxm::vec3(0, 1, 1));
            gvp.getDebugDraw().circle(origin, 1.0f, gfxm::vec3(0,1,0));
        }*/

/*
        if(_resource->root_motion_node_index >= 0) {
            if(_resource->enable_root_motion_t_xz) {
                int32_t rm_bone_id = mapping[_resource->root_motion_node_index];

                int root_of_rm_src_index = ref_skel->getRootOf(rm_bone_id);
                
                gfxm::mat4 m_rm_parent = ::getParentTransform(ref_skel.get(), pose, rm_bone_id);
                gfxm::vec3 rm_world_t = m_rm_parent * gfxm::vec4(pose[rm_bone_id].t, 1.0f);
                gfxm::vec3 rm_parent_world_t = m_rm_parent * gfxm::vec4(.0f, .0f, .0f, 1.0f);

                gvp.getDebugDraw().point(root->getTransform()->getWorldTransform() * gfxm::vec4(rm_world_t, 1.0f), gfxm::vec3(1,0,0));
                gvp.getDebugDraw().point(root->getTransform()->getWorldTransform() * gfxm::vec4(rm_parent_world_t, 1.0f), gfxm::vec3(0,1,0));

                auto& t = pose[root_of_rm_src_index].t;
                t.x -= rm_world_t.x;
                t.z -= rm_world_t.z;

                gvp.getDebugDraw().point(root->getTransform()->getWorldTransform() * gfxm::vec4(t, 1.0f), gfxm::vec3(1,1,0));
            }
        }*/

        // ===

        tgt_nodes.resize(skel->boneCount());
        for(size_t i = 0; i < skel->boneCount(); ++i) {
            auto& bone = skel->getBone(i);
            ecsEntityHandle hdl = world.findEntity(bone.name.c_str());
            tgt_nodes[i] = hdl;
        }
        for(size_t i = 0; i < sample_buf.sampleCount(); ++i) {
            auto n = tgt_nodes[i];
            if(n.isValid()) {
                n.getAttrib<ecsTranslation>()->setPosition(sample_buf[i].t);
                n.getAttrib<ecsRotation>()->setRotation(sample_buf[i].r);
                n.getAttrib<ecsScale>()->setScale(sample_buf[i].s);
            }
        }

        if(_resource->enable_root_motion_t_xz && _resource->root_motion_node_index >= 0 && hRoot) {
            gfxm::vec3 rm_t = sample_buf.getRootMotionDelta().t;
            rm_t.y = .0f;
            hRoot.getAttrib<ecsTranslation>()->translate(rm_t);
        }

        /*
        if(root_motion_bone_id >= 0 && root_motion_node) {
            gfxm::vec3 t = rm_zero_sample.t;
            t.y = pose[root_motion_bone_id].t.y;
            root_motion_node->getTransform()->setPosition(t);
            
            root_motion_node->getTransform()->rotate(gfxm::inverse(zero_delta_rotation));
            //root_motion_node->getTransform()->setRotation(q);
            //root_motion_node->getTransform()->setRotation(rm_zero_sample.r);
        }*/
    }

    hLight.getAttrib<ecsRotation>()->setRotation(gfxm::to_quat(gfxm::to_orient_mat3(gfxm::inverse(gvp.getView()))));
    hLight.getAttrib<ecsWorldTransform>();

    world.update(dt);

    auto window = ImGui::GetCurrentWindow();
    auto rect = window->ContentsRegionRect;

    ImGuiContext* ctx = ImGui::GetCurrentContext();
    const ImGuiStyle& style = ctx->Style;
    float button_height = style.FramePadding.y * 2.0f;
    button_height += ImGui::GetTextLineHeightWithSpacing();
    float timeline_height = 50.0f;
    float viewport_height = (rect.Max.y - rect.Max.y) - timeline_height - button_height;

    if(cam_pivot) {
        gvp.camSetPivot(cam_pivot.getAttrib<ecsWorldTransform>()->getPosition());
    }
    gvp.camMode(GuiViewport::CAM_ORBIT);
    
    DrawList dl;
    world.getSystem<ecsysSceneGraph>();
    auto renderSys = world.getSystem<ecsRenderSystem>();
    renderSys->fillDrawList(dl);
    if (gvp.begin(gfxm::ivec2(0, viewport_height))) {
        gvp.getRenderer()->draw(gvp.getViewport(), gvp.getProjection(), gvp.getView(), dl);
    }
    gvp.end();

    ImGui::Button(ICON_MDI_SKIP_BACKWARD);
    ImGui::SameLine();
    if(ImGui::Button(playing ? ICON_MDI_PAUSE : ICON_MDI_PLAY)) {
        playing = !playing;
    }
    ImGui::SameLine();
    ImGui::Button(ICON_MDI_FAST_FORWARD);
    ImGui::SameLine();
    if(ImGui::Button("Reset offset")) {
        hRoot.getAttrib<ecsTranslation>()->setPosition(gfxm::vec3(0,0,0));
    }
    ImGui::SameLine();
    ImGui::Text(MKSTR(cursor).c_str());
    
    float cur = cursor;
    if(ImGuiExt::BeginTimeline(_resource->length, &cur)) {
        ImGuiExt::TimelineEvent("event_test", 11);
        ImGuiExt::TimelineMarker("marker", 16);
        ImGuiExt::EndTimeline();
    }

    if(!playing) {
        cursor = cur;
    }
}  

void DocAnim::onGuiToolbox(Editor* ed) {
    if(ImGui::DragFloat("playback speed", &playback_speed, 0.01f)) {

    }
    imguiResourceTreeCombo("reference", ref_object, "ecsw", [this](){
        if(ref_object) {
            cam_pivot = ecsEntityHandle();
            world.clearEntities();
            hRoot = world.mergeWorld(ref_object.get());
            hRoot.getAttrib<ecsTranslation>();
            hRoot.getAttrib<ecsRotation>();
            hRoot.getAttrib<ecsWorldTransform>();

            hLight = world.createEntity();
            hLight.getAttrib<ecsLightDirect>()->intensity = 25.0f;

            _resource->reference_file = ref_object->Name();
        }
    });
    imguiResourceTreeCombo("skeleton", ref_skel, "skl", [this](){
        if(ref_skel) {
            _resource->reference_skeleton_file = ref_skel->Name();
        }
    });
    if(imguiEntityCombo("camera pivot", &world, cam_pivot)) {

    }

    ImGui::Separator();

    ImGui::Checkbox("Root motion translation XZ", &_resource->enable_root_motion_t_xz);
    ImGui::Checkbox("Root motion translation Y", &_resource->enable_root_motion_t_y);
    ImGui::Checkbox("Root motion rotation Y", &_resource->enable_root_motion_r_y);

    std::string root_motion_node_name = "<null>";
    if(!_resource->root_motion_node_name.empty()) {
        root_motion_node_name = _resource->root_motion_node_name;
    }
    if(ImGui::BeginCombo("root motion node", root_motion_node_name.c_str())) {
        if(ImGui::Selectable("<null>", _resource->root_motion_node_name.empty())) {
            _resource->setRootMotionSourceNode("");
        }

        for(int i = 0; i < _resource->nodeCount(); ++i) {
            std::string name = _resource->getNodeName(i).c_str();
            if(ImGui::Selectable(name.c_str(), name == root_motion_node_name)) {
                _resource->setRootMotionSourceNode(name);
            }
        }

        ImGui::EndCombo();
    }
}

void DocAnim::onResourceSet() {
    // Set reference
    // Set ref skeleton
    // Set camera target
    // Set camera rotations
}