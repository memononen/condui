#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_internal.h" // for HoveredIdTimer
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>


GLFWwindow* g_window;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
bool show_demo_window = true;
bool show_window = true;
bool show_help_window = true;
int g_width;
int g_height;

EM_JS(int, canvas_get_width, (), {
  return Module.canvas.width;
});

EM_JS(int, canvas_get_height, (), {
  return Module.canvas.height;
});

EM_JS(void, resizeCanvas, (), {
  js_resizeCanvas();
});

void on_size_changed()
{
  glfwSetWindowSize(g_window, g_width, g_height);

  ImGui::SetCurrentContext(ImGui::GetCurrentContext());
}

typedef unsigned char uint8;
typedef signed char int8;

enum class Operand : uint8 { Copy = 0, And, Or, };
const char* opName[] = { "=", "AND", "OR", };
const char* opNameShort[] = { " ", "&", "|", };

constexpr uint8 MaxIndent = 4;

struct Cond
{
    Cond()
    {
      strcpy(name, "Name");
    }

    void set(int8 _indent, Operand _op, const char* _name)
    {
      indent = _indent;
      op = _op;
      strcpy(name, _name);
    }

    Operand op = Operand::And;
    int8 indent = 0;
    char name[64];
};


void pushDimButtonStyle(bool dimText = false)
{
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0.3f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.1f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.2f));
  if (dimText)
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,0.3f));
  else
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,0.8f));
}
                
void popDimButtonStyle()
{
  ImGui::PopStyleColor(4);
}

void pushLightButtonStyle()
{
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,1,1,0.1f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.2f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.3f));
}
                
void popLightButtonStyle()
{
  ImGui::PopStyleColor(3);
}

void pushOperandButtonStyle(const Operand op)
{
  ImVec4 col;
  if (op == Operand::And)
    col = ImVec4(0.75f, 0.5f, 1.0f, 1.0f);
  else
    col = ImVec4(0.75f, 1.0f, 0.5f, 1.0f);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x, col.y, col.z, 0.5f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col.x, col.y, col.z, 0.6f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col.x, col.y, col.z, 0.7f));
}
                
void popOperandButtonStyle()
{
  ImGui::PopStyleColor(3);
}

bool shouldShowTooltip()
{
  return  ImGui::IsItemHovered()
          && ImGui::GetCurrentContext()->HoveredIdTimer > 1.0f
          && !ImGui::IsItemFocused();
}


void loop()
{
  int width = canvas_get_width();
  int height = canvas_get_height();

  if (width != g_width || height != g_height)
  {
    g_width = width;
    g_height = height;
    on_size_changed();
  }

  glfwPollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();


  constexpr int MaxConditions = 12;

  static Cond conditions[MaxConditions];
  static int numConditions = 0;

  static bool useIndentDropdown = false;

  static bool initDone = false;
  if (!initDone)
  {
    conditions[0].set(0, Operand::And, "Is Hungry");
    conditions[1].set(1, Operand::Or, "Is Thirsty");
    conditions[2].set(0, Operand::And, "Is Dog");
    conditions[3].set(2, Operand::Or, "Is Cat");
    conditions[4].set(1, Operand::And, "Is Green");
    numConditions = 5;

    initDone = true;
  }

  ImGui::SetNextWindowPos(ImVec2(50, 20), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
  ImGui::Begin("Conditions", &show_window);

  ImGui::Separator();

  ImVec2 butSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());

  ImGui::Text("Conditions");

  ImGui::SameLine();
  if (ImGui::Button("+", butSize))
  {
      if (numConditions < MaxConditions)
      {
        numConditions++;
      }
  }
  ImGui::SameLine();
  if (ImGui::Button("-", butSize))
  {
      if (numConditions > 0)
      {
        numConditions--;
      }
  }

  ImGui::Separator();

  char label[128];
  ImGuiStyle& style = ImGui::GetStyle();

  for (int i = 0; i < numConditions; i++)
  {
    Cond& c = conditions[i];

    const int currIndent = i == 0 ? 0 : c.indent;
    const int nextIndent = (i + 1) < numConditions ? conditions[i+1].indent : 0;
    const int delta = nextIndent - currIndent;
    const int openParens = delta > 0 ? delta : 0;
    const int closedParens = delta < 0 ? -delta : 0;

    ImGui::PushID(i);

    ImGui::BeginGroup();

    // indent
    const float indentStep = ImGui::GetFontSize() * 2;

    if (useIndentDropdown)
    {
      // UI option 1, use a dropdown.
      const ImVec2 indentSize(ImGui::GetFontSize() + (indentStep + style.ItemSpacing.x) * c.indent, ImGui::GetFrameHeight());

      pushDimButtonStyle();
      if (ImGui::Button("##indent", indentSize))
      {
        ImGui::OpenPopup("cond_indent");
      }
      popDimButtonStyle();

      if (ImGui::BeginPopup("cond_indent"))
      {
        for (int j = 0; j < MaxIndent; j++)
        {
          ImGui::PushID(j);
          snprintf(label, IM_ARRAYSIZE(label), "%d", j);
          if (ImGui::Selectable(label, c.indent == j))
              c.indent = j;
          ImGui::PopID();
        }
        ImGui::EndPopup();
      }
    }
    else
    {
      // UI option 2, use buttons, one for add, and each indent is remove.
      const ImVec2 indentPlusSize(ImGui::GetFontSize(), ImGui::GetFrameHeight());
      const ImVec2 indentMinusSize(indentStep, ImGui::GetFrameHeight());

      pushDimButtonStyle(/*dimText*/true);
      if (ImGui::Button(">##indent+", indentPlusSize))
      {
        if ((c.indent+1) < MaxIndent)
          c.indent++;
      }
      popDimButtonStyle();
      if (shouldShowTooltip())
      {
        ImGui::SetTooltip("Add indent.");
      }

      for (int j = 0; j < c.indent; j++)
      {
        ImGui::SameLine();
        ImGui::PushID(j);
        pushDimButtonStyle(/*dimText*/true);
        if (ImGui::Button("|##indent-", indentMinusSize))
        {
            c.indent--;
        }
        popDimButtonStyle();
        if (shouldShowTooltip())
        {
          ImGui::SetTooltip("Remove indent.");
        }
        ImGui::PopID();
      }

    }

    // Operand
    ImVec2 operandSize(indentStep, ImGui::GetFrameHeight());

    ImGui::SameLine();
    if (i == 0)
    {
      // There is no operator on first line, but we need to space for things to line up, could be empty too, using IF so that the whole tlooks like condition.
      pushDimButtonStyle();
      ImGui::Button("IF", operandSize);
      popDimButtonStyle();
    }
    else
    {
      pushOperandButtonStyle(c.op);
      if (ImGui::Button(opName[(int)c.op], operandSize))
        ImGui::OpenPopup("cond_op");
      popOperandButtonStyle();
    }

    if (ImGui::BeginPopup("cond_op"))
    {
      if (ImGui::Selectable("AND"))
          c.op = Operand::And;
      if (ImGui::Selectable("OR"))
          c.op = Operand::Or;
      ImGui::EndPopup();
    }

    // open parens
    ImGui::SameLine();
    ImGui::Text("%.*s", openParens, "((((");

    // name
    pushLightButtonStyle();
    ImGui::SameLine();
    snprintf(label, IM_ARRAYSIZE(label), "%s###name", c.name);
    bool focusName = false;
    if (ImGui::Button(label))
    {
      ImGui::OpenPopup("cond_name");
      focusName = true;
    }
    popLightButtonStyle();

    if (ImGui::BeginPopup("cond_name"))
    {
      if (focusName)
        ImGui::SetKeyboardFocusHere();
      if (ImGui::InputText("##edit", c.name, IM_ARRAYSIZE(c.name), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
          ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }

    // closed parens
    ImGui::SameLine();
    ImGui::Text("%.*s", closedParens, "))))");

    // Reorder
    ImVec2 reorderSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - reorderSize.x);

    pushDimButtonStyle();
    ImGui::Button(":", reorderSize);
    popDimButtonStyle();
    if (shouldShowTooltip())
    {
      ImGui::SetTooltip("Drag to reorder the item.");
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
      ImGui::SetDragDropPayload("DND_CONDITION", &i, sizeof(int));
      ImGui::Text("Reorder %s", c.name);
      ImGui::EndDragDropSource();
    }

    ImGui::EndGroup();

    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_CONDITION"))
      {
        IM_ASSERT(payload->DataSize == sizeof(int));
        int from = *(const int*)payload->Data;
        int to = i;
        if (from != to && from >= 0 && from < numConditions)
        {
          // Remove from
          Cond tmp = conditions[from];
          for (int j = from; j < numConditions-1; j++)
            conditions[j] = conditions[j+1];
          // Not adjust ting `to` intentionally, so that the move is directional (insert before or after based on direction).
          // Insert to
          for (int j = numConditions-1; j > to; j--)
            conditions[j] = conditions[j-1];
          conditions[to] = tmp;
        }
      }
      ImGui::EndDragDropTarget();
    }

    ImGui::PopID();

  }

  ImGui::Separator();

  ImGui::End();


  ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
  ImGui::Begin("Help", &show_help_window);

  ImGui::TextWrapped("This is an example of boolean expression editor."); ImGui::Spacing();
  ImGui::TextWrapped("Each condition is at it's own row. The operator is applied between the current and previous condition. No operator precedence, operations are combined in order."); ImGui::Spacing();
  ImGui::TextWrapped("Each line can be indented to create more complicated expressions."); ImGui::Spacing();
  ImGui::TextWrapped("Diff between the idents defines how many braces open or close between the condition lines. The more you indent, the more the line will be associated with the previous line."); ImGui::Spacing();

  ImGui::Separator();
  if (ImGui::RadioButton("Use indent drop down", useIndentDropdown)) useIndentDropdown = true;
  if (ImGui::RadioButton("Use ident buttons", !useIndentDropdown)) useIndentDropdown = false;

  ImGui::End();

  // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
/*  if (show_demo_window)
  {
      ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
      ImGui::ShowDemoWindow(&show_demo_window);
  }*/

  ImGui::Render();

  int display_w, display_h;
  glfwMakeContextCurrent(g_window);
  glfwGetFramebufferSize(g_window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwMakeContextCurrent(g_window);
}


int init_gl()
{
  if( !glfwInit() )
  {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return 1;
  }

  //glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

  // Open a window and create its OpenGL context
  int canvasWidth = 800;
  int canvasHeight = 600;
  g_window = glfwCreateWindow( canvasWidth, canvasHeight, "WebGui Demo", NULL, NULL);
  if( g_window == NULL )
  {
      fprintf( stderr, "Failed to open GLFW window.\n" );
      glfwTerminate();
      return -1;
  }
  glfwMakeContextCurrent(g_window); // Initialize GLEW

  return 0;
}


int init_imgui()
{
  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(g_window, true);
  ImGui_ImplOpenGL3_Init();

  // Setup style
  ImGui::StyleColorsDark();

  ImGuiIO& io = ImGui::GetIO();

  // Load Fonts
//  io.Fonts->AddFontFromFileTTF("data/Chivo-Regular.ttf", 23.0f);
  io.Fonts->AddFontFromFileTTF("data/Chivo-Regular.ttf", 18.0f);
//  io.Fonts->AddFontFromFileTTF("data/Chivo-Regular.ttf", 26.0f);
//  io.Fonts->AddFontFromFileTTF("data/Chivo-Regular.ttf", 32.0f);
  io.Fonts->AddFontDefault();

  resizeCanvas();

  return 0;
}


int init()
{
  init_gl();
  init_imgui();
  return 0;
}


void quit()
{
  glfwTerminate();
}


extern "C" int main(int argc, char** argv)
{
  if (init() != 0) return 1;

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, 0, 1);
  #endif

  quit();

  return 0;
}
