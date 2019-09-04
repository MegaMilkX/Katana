#include "imgui_wrap.hpp"
#include "imgui/imgui_freetype.h"

#include "../gl/shader_program.h"

#include <memory>

#include "../gen/Karla_Regular.ttf.h"
#include "../gen/OpenSans_Regular.ttf.h"
#include "../gen/materialdesignicons_webfont.ttf.h"

#include "../common/util/materialdesign_icons.hpp"


static ImGuiContext* imGuiCtx;
static GLuint imGuiVBuf;
static GLuint imGuiIBuf;
static std::shared_ptr<gl::ShaderProgram> imGuiProgram;
static GLuint imGuiTexture2d;


void ImGuiInit() {
    imGuiCtx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float)1280;
    io.DisplaySize.y = (float)720;
    
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
    io.ConfigWindowsResizeFromEdges = true;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = 0;

    ImGui::SetCurrentContext(imGuiCtx);
    
    io.KeyMap[ImGuiKey_Backspace] = 8;
    io.KeyMap[ImGuiKey_Enter] = 13;

    // TODO: Create texture
    unsigned char* pixels;
    int width, height;
    io.Fonts->AddFontFromMemoryTTF((void*)OpenSans_Regular_ttf, sizeof(OpenSans_Regular_ttf), 16);

    static const ImWchar icons_ranges[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryTTF((void*)materialdesignicons_webfont_ttf, sizeof(materialdesignicons_webfont_ttf), 16, &icons_config, icons_ranges);

    ImGuiFreeType::BuildFontAtlas(io.Fonts);

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    glGenTextures(1, &imGuiTexture2d);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, imGuiTexture2d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*)pixels);

    io.Fonts->TexID = (void*)imGuiTexture2d;

    // TODO: Create shader program
    gl::Shader vs(GL_VERTEX_SHADER);
    gl::Shader fs(GL_FRAGMENT_SHADER);
    vs.source("#version 450\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n"
    );
    fs.source("#version 450\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n");
    vs.compile();
    fs.compile();
    imGuiProgram.reset(new gl::ShaderProgram());
    imGuiProgram->attachShader(&vs);
    imGuiProgram->attachShader(&fs);
    imGuiProgram->bindAttrib(0, "Position");
    imGuiProgram->bindAttrib(1, "UV");
    imGuiProgram->bindAttrib(2, "Color");
    imGuiProgram->bindFragData(0, "Out_Color");
    imGuiProgram->link();
    imGuiProgram->use();
    glUniform1i(imGuiProgram->getUniform("Texture"), 0);

    // TODO: Create buffers
    glGenBuffers(1, &imGuiVBuf);
    glGenBuffers(1, &imGuiIBuf);
}
void ImGuiCleanup() {
    glDeleteTextures(1, &imGuiTexture2d);
}
void ImGuiUpdate(float dt, int w, int h) {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = dt;
    io.DisplaySize.x = (float)w;
    io.DisplaySize.y = (float)h;
    ImGui::NewFrame();
}
void ImGuiDraw() {
    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    ImDrawData* draw_data = ImGui::GetDrawData();
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

    float proj[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    float left = 0.0f;
    float right = io.DisplaySize.x;
    float bottom = io.DisplaySize.y;
    float top = 0.0f;
    float znear = -1.0f;
    float zfar = 1.0f;
    proj[0][0] = 2.0f / (right - left);
    proj[1][1] = 2.0f / (top - bottom);
    proj[2][2] = -2.0f / (zfar - znear);
    proj[3][0] = -(right + left) / (right - left);
    proj[3][1] = -(top + bottom) / (top - bottom);
    proj[3][2] = -(zfar + znear) / (zfar - znear);

    imGuiProgram->use();
    glUniformMatrix4fv(imGuiProgram->getUniform("ProjMtx"), 1, GL_FALSE, (const GLfloat*)proj);
    GL_LOG_ERROR("glUniformMatrix4fv");

    GLuint vao_handle = 0;
    glGenVertexArrays(1, &vao_handle);
    glBindVertexArray(vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, imGuiVBuf);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));


    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, imGuiVBuf);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imGuiIBuf);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)pcmd->TextureId);
                GL_LOG_ERROR("glBindTexture");
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
                GL_LOG_ERROR("glDrawElements");
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glDeleteVertexArrays(1, &vao_handle);
}