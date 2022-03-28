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

enum class Operator : uint8 { Copy = 0, And, Or, };
const char* opName[] = { "=", "AND", "OR", };
const char* opNameShort[] = { " ", "&", "|", };

constexpr uint8 MaxIndent = 4;

struct Cond
{
    Cond()
    {
      strcpy(name, "Name");
    }

    bool eval() const { return val; }

    void set(int8 _indent, Operator _op, const char* _name)
    {
      indent = _indent;
      op = _op;
      strcpy(name, _name);
    }

    Operator op = Operator::And;
    int8 indent = 0;
    char name[64];
    bool val = false;
};

void pushDimTextStyle()
{
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,0.3f));
}
                
void popDimTextStyle()
{
  ImGui::PopStyleColor(1);
}

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

void pushOperatorButtonStyle(const Operator op)
{
  ImVec4 col;
  if (op == Operator::And)
    col = ImVec4(0.75f, 0.5f, 1.0f, 1.0f);
  else
    col = ImVec4(0.75f, 1.0f, 0.5f, 1.0f);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x, col.y, col.z, 0.5f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col.x, col.y, col.z, 0.6f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col.x, col.y, col.z, 0.7f));
}
                
void popOperatorButtonStyle()
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
    conditions[0].set(0, Operator::And, "IsDog");
    conditions[1].set(2, Operator::Or, "IsCat");
    conditions[2].set(0, Operator::And, "IsNearHome");
    conditions[3].set(1, Operator::And, "IsHungry");
    conditions[4].set(2, Operator::Or, "IsIsThirsty");
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
      if (ImGui::Button("+##indent+", indentPlusSize))
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
        if (ImGui::Button("·##indent-", indentMinusSize))
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

    // Operator
    ImVec2 operatorSize(indentStep, ImGui::GetFrameHeight());

    ImGui::SameLine();
    if (i == 0)
    {
      // There is no operator on first line, but we need to space for things to line up, could be empty too, using IF so that the whole tlooks like condition.
      pushDimButtonStyle();
      ImGui::Button("IF", operatorSize);
      popDimButtonStyle();
    }
    else
    {
      pushOperatorButtonStyle(c.op);
      if (ImGui::Button(opName[(int)c.op], operatorSize))
        ImGui::OpenPopup("cond_op");
      popOperatorButtonStyle();
    }

    if (ImGui::BeginPopup("cond_op"))
    {
      if (ImGui::Selectable("AND"))
          c.op = Operator::And;
      if (ImGui::Selectable("OR"))
          c.op = Operator::Or;
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

    // Value
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetFrameHeight() * 3);
    ImGui::Checkbox("##val", &c.val);
    if (shouldShowTooltip())
    {
      ImGui::SetTooltip("Value to be used for debug evaluation below.");
    }

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

  // Eval

  ImGui::Text("Evaluation");

  bool vals[MaxIndent+1] = { false, false, false, false, false };
  Operator ops[MaxIndent+1] = { Operator::Copy, Operator::Copy, Operator::Copy, Operator::Copy, Operator::Copy };

  int level = 0;

  for (int i = 0; i < numConditions; i++)
  {
    const Cond& c = conditions[i];

    // First one is copy, as there's no previous value.
    Operator currOp = i == 0 ? Operator::Copy : c.op;
    // These can be precalculated
    const int currIndent = i == 0 ? 0 : c.indent;
    const int nextIndent = (i + 1) < numConditions ? conditions[i+1].indent : 0;
    const int delta = nextIndent - currIndent;
    // +1 is used to store the current value at the top of the vals stack.
    const int openParens = (delta > 0 ? delta : 0) + 1;
    const int closedParens = (delta < 0 ? -delta : 0) + 1;

    // Store how to combine the upper level value down when coming back to this level
    ops[level] = currOp;

    // Evaluate condition and store result
    level += openParens;
    vals[level] = c.eval();

    ImGui::Text("[%d] = %s", level, c.name);
    ImGui::SameLine();
    pushDimTextStyle();
    ImGui::Text("%s", c.val ? "True" : "False");
    popDimTextStyle();

    // Combine stack down.
    ImGui::Indent();
    for (int j = 0; j < closedParens; j++)
    {
      level--;

      ImGui::Text("[%d]", level);
      ImGui::SameLine();
      pushDimTextStyle();
      ImGui::Text("%s", vals[level] ? "True" : "False");
      popDimTextStyle();
      ImGui::SameLine();
      ImGui::Text("%s= [%d]", opNameShort[(int)ops[level]], level+1);
      ImGui::SameLine();
      pushDimTextStyle();
      ImGui::Text("%s", vals[level+1] ? "True" : "False");
      popDimTextStyle();

      switch (ops[level])
      {
        case Operator::Copy:
          vals[level] = vals[level+1];
          break;
        case Operator::And:
          vals[level] &= vals[level+1];
          break;
        case Operator::Or:
          vals[level] |= vals[level+1];
          break;
      }
      ops[level] = Operator::Copy;
    }
    ImGui::Unindent();
  }

  ImGui::Text("Result = ");
  ImGui::SameLine();
  pushDimTextStyle();
  ImGui::Text("%s", vals[0] ? "True" : "False");
  popDimTextStyle();

  ImGui::End();


  ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
  ImGui::Begin("Help", &show_help_window);

  ImGui::TextWrapped("This is an example of a simple boolean expression editor and evaluation."); ImGui::Spacing();
  ImGui::TextWrapped("Conditions are on individual rows. The operator is applied between the current and previous condition, no operator precedence."); ImGui::Spacing();
  ImGui::TextWrapped("Each line can be indented to add parenthesis and to create more complicated expressions."); ImGui::Spacing();
  ImGui::TextWrapped("Difference in the identation between the condition lines defines how many paretheses are opened or closed. One way to think about this is that, the more you indent, the more the line will be associated with the previous line."); ImGui::Spacing();

  ImGui::Separator();
  ImGui::TextWrapped("The indentation UI can be made in many different ways. Here are two examples, using a drop down or buttons."); ImGui::Spacing();
  if (ImGui::RadioButton("Use indent drop down", useIndentDropdown)) useIndentDropdown = true;
  if (ImGui::RadioButton("Use indent buttons", !useIndentDropdown)) useIndentDropdown = false;

  ImGui::End();

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
