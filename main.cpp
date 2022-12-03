#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#endif

#define GLFW_INCLUDE_GLEXT
#define GLFW_INCLUDE_GLCOREARB
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
int g_width = 0;
int g_height = 0;

EM_JS(int, canvas_get_width, (), {
  return Module.canvas.clientWidth;
});

EM_JS(int, canvas_get_height, (), {
  return Module.canvas.clientHeight;
});

typedef unsigned char uint8;
typedef signed char int8;

enum class Operator : uint8 { Copy = 0, And, Or, };
const char* opName[] = { "CP", "AND", "OR", };
const char* opNameShort[] = { " ", "&", "|", };

constexpr uint8 MaxExpressionDepth = 4;

bool apply(const Operator op, const bool lhs, const bool rhs)
{
  if (op == Operator::And)
    return lhs && rhs;
  if (op == Operator::Or)
    return lhs || rhs;
  return rhs; // copy
}

int maxi(int a, int b) { return a > b ? a : b; }
int mini(int a, int b) { return a < b ? a : b; }

template <typename T, int N>
void fill(T (&arr)[N], const T v)
{
    for (int i = 0; i < N; i++)
        arr[i] = v;
}


struct Cond
{
    Cond()
    {
      strcpy(name, "Name");
    }

    bool eval() const { return val; }

    void set(int8 _depth, Operator _op, const char* _name)
    {
      depth = _depth;
      op = _op;
      strcpy(name, _name);
    }

    Operator op = Operator::And;
    int8 depth = 0;     // current expression depth, this is easier to understand in the UI
    int8 nextDepth = 0; // next items expression depth, this is used during evaluation
    char name[64];
    bool val = false;
};

// Find next operator in the same sub-expression
int findNextOpIndex(const Cond* conds, const int numConds, int index)
{
  const int initialDepth = conds[index].depth;
  for (int i = index+1; i < numConds; i++)
  {
    if (conds[i].depth < initialDepth)
      return -1;
    if (conds[i].depth == initialDepth)
      return i;
  }
  return -1;
}

// Finds first operator in the same sub-expression.
int getFirstOperatorIndex(const Cond* conds, const int numConds, int index)
{
  int result = index;
  const int initialDepth = conds[index].depth; 
  for (int i = index-1; i > 0; i--) // omitting 0, as it does not have operator.
  {
    if (conds[i].depth < initialDepth)
      break;
    if (conds[i].depth == initialDepth && conds[i].op != Operator::Copy)
      result = i;
  }
  return result;
}

void pushDimTextStyle()
{
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,0.3f));
}
                
void popDimTextStyle()
{
  ImGui::PopStyleColor(1);
}

void pushRedTextStyle()
{
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0.2f,0.2f,0.8f));
}
                
void popRedTextStyle()
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

void pushNonButtonStyle()
{
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));
}
                
void popNonButtonStyle()
{
  ImGui::PopStyleColor(3);
}

void pushWarningButtonStyle()
{
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0.7f,0.2f,0.7f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,0.7f,0.2,0.9f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,0.7f,0.2,0.9f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,0,0,1));
}
                
void popWarningButtonStyle()
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

void dumpState(const int opDepth, const int valDepth, const Operator* ops, const bool* vals, const int count)
{
  ImGui::TableNextRow();

  // dummy
  ImGui::TableNextColumn();

  for (int i = 0; i < count; i++)
  {
    ImGui::TableNextColumn();

    if (i <= valDepth)
    {
      if (i != opDepth)
        pushDimTextStyle();

      ImGui::Text("%s", opNameShort[(int)ops[i]]);

      if (i != opDepth)
        popDimTextStyle();

      if (i != valDepth)
        pushDimTextStyle();

      ImGui::SameLine();
      ImGui::Text("%s", vals[i] ? "T" : "F");

      if (i != valDepth)
        popDimTextStyle();
    }
  }

  ImGui::TableNextColumn();
}


void loop()
{
#ifdef __EMSCRIPTEN__

  int width = canvas_get_width();
  int height = canvas_get_height();

  if (width != g_width || height != g_height)
  {
    g_width = width;
    g_height = height;

    glfwSetWindowSize(g_window, g_width, g_height);

    ImGui::SetCurrentContext(ImGui::GetCurrentContext());
  }
#endif

  glfwPollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();

  ImGui::NewFrame();


  constexpr int MaxConditions = 12;

  static Cond conditions[MaxConditions];
  static int numConditions = 0;

  static bool useDepthDropdown = false;
  static bool usePrecedence = false;
  static bool guideOperatorUsage = true;
  static bool useShortCircuit = true;

  static bool initDone = false;
  if (!initDone)
  {
    conditions[0].set(0, Operator::And, "IsDog");
    conditions[1].set(1, Operator::Or, "IsCat");
    conditions[2].set(0, Operator::And, "IsNearHome");
    conditions[3].set(1, Operator::And, "IsHungry");
    conditions[4].set(2, Operator::Or, "IsThirsty");

    numConditions = 5;

    initDone = true;
  }

  ImGui::SetNextWindowPos(ImVec2(50, 20), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
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

  bool precedence[MaxExpressionDepth];   // true if specific depth is processing in high precedence operator 
  bool pendingClose[MaxExpressionDepth]; // true if specific depth should close parenthesis (due to high precedence operator) after the children has been processed
  fill(precedence, false);
  fill(pendingClose, false);


  const float indentStep = ImGui::GetFontSize() * 2;
  const float smallIndentStep = ImGui::GetFontSize();

  for (int i = 0; i < numConditions; i++)
  {
    Cond& cond = conditions[i];

    const int currDepth = i == 0 ? 0 : cond.depth;
    const int nextDepth = (i + 1) < numConditions ? conditions[i+1].depth : 0;
    const int delta = nextDepth - currDepth;
    const int openParens = delta > 0 ? delta : 0;
    const int closedParens = delta < 0 ? -delta : 0;

    const int nextOpIndex = findNextOpIndex(conditions, numConditions, i);
    const Operator nextOp = nextOpIndex != -1 ? conditions[nextOpIndex].op : Operator::Copy;

    ImGui::PushID(i);

    ImGui::BeginGroup();

    // indent from depth
    if (i > 0)
    {
      if (useDepthDropdown)
      {
        // UI option 1, use a dropdown.
        const ImVec2 indentSize(ImGui::GetFontSize() + (indentStep + style.ItemSpacing.x) * cond.depth, ImGui::GetFrameHeight());

        pushDimButtonStyle();
        if (ImGui::Button("##depth", indentSize))
        {
          ImGui::OpenPopup("cond_depth");
        }
        popDimButtonStyle();

        if (ImGui::BeginPopup("cond_depth"))
        {
          for (int j = 0; j < MaxExpressionDepth; j++)
          {
            ImGui::PushID(j);
            snprintf(label, IM_ARRAYSIZE(label), "%d", j);
            if (ImGui::Selectable(label, cond.depth == j))
                cond.depth = j;
            ImGui::PopID();
          }
          ImGui::EndPopup();
        }
      }
      else
      {
        const ImVec2 incSize(smallIndentStep, ImGui::GetFrameHeight());
        const ImVec2 decSize(indentStep, ImGui::GetFrameHeight());

        if (i > 0)
        {
          // UI option 2, use buttons, one for add, and each depth step has remove.
          pushDimButtonStyle(true);
          if (ImGui::Button("+##depth+", incSize))
          {
            if ((cond.depth+1) < MaxExpressionDepth)
              cond.depth++;
          }
          popDimButtonStyle();
          if (shouldShowTooltip())
          {
            ImGui::SetTooltip("Increase operand depth.");
          }

          for (int j = 0; j < cond.depth; j++)
          {
            ImGui::SameLine();
            ImGui::PushID(j);
            pushDimButtonStyle(/*dimText*/true);
            if (ImGui::Button("-##depth-", decSize))
            {
                cond.depth--;
            }
            popDimButtonStyle();

            if (shouldShowTooltip())
            {
              ImGui::SetTooltip("Decrease operand depth.");
            }

            ImGui::PopID();
          }
        }
      }
    }
    else
    {
      const ImVec2 incSize(smallIndentStep, ImGui::GetFrameHeight());

      pushNonButtonStyle();
      ImGui::Button("", incSize);
      popNonButtonStyle();
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
      pushOperatorButtonStyle(cond.op);
      if (ImGui::Button(opName[(int)cond.op], operatorSize))
        ImGui::OpenPopup("cond_op");
      popOperatorButtonStyle();
    }

    if (ImGui::BeginPopup("cond_op"))
    {
      if (ImGui::Selectable("AND"))
          cond.op = Operator::And;
      if (ImGui::Selectable("OR"))
          cond.op = Operator::Or;
      ImGui::EndPopup();
    }

    if (usePrecedence)
    {
      // Extra parens from precedence
      if (!precedence[currDepth])
      {
        if (cond.op == Operator::Or && nextOp == Operator::And)
        {
          pushDimTextStyle();
          ImGui::SameLine();
          ImGui::Text("(");
          popDimTextStyle();

          precedence[currDepth] = true;
        }
      }
    }
    else if (guideOperatorUsage)
    {
      const int firstOpIndex = getFirstOperatorIndex(conditions, numConditions, i);
      const Operator firstOp = conditions[firstOpIndex].op;
      if (firstOp != conditions[i].op)
      {
        // Warn about different operators.
        pushWarningButtonStyle();
        ImGui::SameLine();
        if (ImGui::Button("!"))
        {
          conditions[i].op = firstOp;
        }
        popWarningButtonStyle();
        if (shouldShowTooltip())
        {
          ImGui::SetTooltip("The operator is different from others in the sub-expression.\nThis can cause misinterpretation of the expression.\nClick to use the same operator as above.");
        }
      }
    }

    // open parens
    for (int j = 0; j < openParens-1; j++)
    {
      ImGui::SameLine();
      pushNonButtonStyle();
      ImGui::Button("(", operatorSize);
      popNonButtonStyle();
    }

    // the last parent goes at the same depth as the operand, associates better visually.
    if (openParens > 0)
    {
      ImGui::SameLine();
      ImGui::Text("(");
    }

    // name
    pushLightButtonStyle();
    ImGui::SameLine();
    snprintf(label, IM_ARRAYSIZE(label), "%s###name", cond.name);
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
      if (ImGui::InputText("##edit", cond.name, IM_ARRAYSIZE(cond.name), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
          ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }

    // closed parens
    if (usePrecedence)
    {
      // Interleave parens from depth and precedence.
      for (int j = 0; j < closedParens; j++)
      {
        if (precedence[currDepth-j])
        {
          pushDimTextStyle();
          ImGui::SameLine();
          ImGui::Text(")");
          popDimTextStyle();

          precedence[currDepth-j] = false;
        }

        ImGui::SameLine();
        ImGui::Text(")");
      }

      if (precedence[currDepth])
      {
        if (cond.op == Operator::And && nextOp != Operator::And)
        {
          if (openParens == 0)
          {
            pushDimTextStyle();
            ImGui::SameLine();
            ImGui::Text(")");
            popDimTextStyle();
            precedence[currDepth] = false;
          }
          else
            pendingClose[currDepth] = true;
        }
      }

      if (closedParens > 0 && pendingClose[currDepth-closedParens])
      {
        pushDimTextStyle();
        ImGui::SameLine();
        ImGui::Text(")");
        popDimTextStyle();
        precedence[currDepth-closedParens] = false;
        pendingClose[currDepth-closedParens] = false;
      }
    }
    else
    {
      for (int j = 0; j < closedParens; j++)
      {
        ImGui::SameLine();
        ImGui::Text(")");
      }
    }

    // Value
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetFrameHeight() * 3);
    ImGui::Checkbox("##val", &cond.val);
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
      ImGui::Text("Reorder %s", cond.name);
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

  // Precalculate nextDepth, and apply precedence.
  fill(precedence, false);
  fill(pendingClose, false);

  int maxDepth = 0;
  int precedenceDepth = 0; // extra depth added by precedence

  for (int i = 0; i < numConditions; i++)
  {
    Cond& cond = conditions[i];
    const bool nextValid = (i + 1) < numConditions;

    const int currDepth = conditions[i].depth;
    const int nextDepth = nextValid ? conditions[i+1].depth : 0;
    const int nextOpIndex = findNextOpIndex(conditions, numConditions, i);
    const Operator nextOp = nextOpIndex != -1 ? conditions[nextOpIndex].op : Operator::Copy;

    maxDepth = maxi(maxDepth, currDepth);

    if (usePrecedence)
    {
      // add paren due to operator precedence
      if (!precedence[currDepth])
      {
        if (cond.op == Operator::Or && nextOp == Operator::And)
        {
          precedence[currDepth] = true;
          precedenceDepth++;
        }
      }

      // close precedence due to closing parens.
      for (int j = currDepth; j > nextDepth; j--)
      {
        if (precedence[j])
        {
          precedence[j] = false;
          precedenceDepth--;
        }
      }

      // close precedence due to starting "or" or end of (sub)expression.
      if (precedence[currDepth])
      {
        if (cond.op == Operator::And && nextOp != Operator::And)
        {
          if (nextDepth >= currDepth)
            precedence[currDepth] = false;
          else
            pendingClose[currDepth] = true; // should close, but this line open sub-expression, postpone until closed.
          precedenceDepth--;
        }
      }

      // close pending precedence, this is done when we detect that we should close
      // precedence at the beginning of a sub-expression, the paren goes after the sub-expression.
      if (nextDepth < currDepth && pendingClose[nextDepth])
      {
        precedence[nextDepth] = false;
        pendingClose[nextDepth] = false;
      }
    }

    cond.nextDepth = nextValid ? (conditions[i+1].depth + precedenceDepth) : 0;
  }

  const int depthCount = maxDepth+1;

  ImGui::Text("Evaluation");
  ImGui::BeginTable("eval", 1 + depthCount + 1, ImGuiTableFlags_BordersInnerV);
  ImGui::TableSetupColumn("pad", ImGuiTableColumnFlags_WidthFixed, smallIndentStep);
  for (int i = 0; i < depthCount; i++)
  {
    snprintf(label, IM_ARRAYSIZE(label), "depth%d", i);
    ImGui::TableSetupColumn(label, ImGuiTableColumnFlags_WidthFixed, indentStep);
  }
  ImGui::TableSetupColumn("res", ImGuiTableColumnFlags_WidthStretch);

  bool vals[MaxExpressionDepth*2+1];
  Operator ops[MaxExpressionDepth*2+1];
  fill(vals, false);
  fill(ops, Operator::Copy);

  constexpr int NoSkip = MaxExpressionDepth+1;
  int depth = 0;
  int skipDepth = NoSkip;

  for (int i = 0; i < numConditions; i++)
  {
    const Cond& cond = conditions[i];
    const Operator op = (i == 0) ? Operator::Copy : cond.op;

    // Store how to combine the higher expression depth value down when coming back to this depth level.
    ops[depth] = op;

    const int opDepth = depth;

    // short circuit
    // if the right hand side of the expression can be skipped because we know the result
    // from current value and operator, skip evaluating the right operand, which means
    // we can skip the whole right sub-expression too. This is done here by just not
    // evaluating anything with higher depth than the current.
    if (useShortCircuit)
    {
      if (depth <= skipDepth)
      {
        if (ops[depth] == Operator::And && vals[depth] == false)
          skipDepth = depth;
        else if (ops[depth] == Operator::Or && vals[depth] == true)
          skipDepth = depth;
        else
          skipDepth = NoSkip;
      }
    }

    // increase eval depth if needed. (nextDepth - depth) = number of open parenthesis.
    if (cond.nextDepth > depth)
      depth = cond.nextDepth;

    // skipping does not really speed up this eval loop, but we expect that the eval() is an expensive
    // call (virtuals, access other systems, etc), which we try to avoid.
    const bool canSkipEval = depth >= skipDepth;

    // Evaluate
    bool val = canSkipEval ? false : cond.eval();

    Operator currOp = ops[depth];

    // Apply the current operand with operator.
    vals[depth] = apply(ops[depth], vals[depth], val);
    ops[depth] = Operator::Copy;

    dumpState(opDepth == depth ? -1 : opDepth, depth, ops, vals, depthCount);

    ImGui::Text("[%d]", depth);
    ImGui::SameLine();
    ImGui::Text("%s= %s", opNameShort[(int)currOp], cond.name);
    if (canSkipEval)
    {
      ImGui::SameLine();
      pushRedTextStyle();
      ImGui::Text("Skip Eval");
      popRedTextStyle();
    }

    // Combine stack down. (depth - nextDepth) = number of closing parenthesis.
    while (depth > cond.nextDepth)
    {
      depth--;

      currOp = ops[depth];

      vals[depth] = apply(ops[depth], vals[depth], vals[depth+1]);
      ops[depth] = Operator::Copy;

      dumpState(depth, depth, ops, vals, depthCount);

      ImGui::Indent();

      ImGui::Text("[%d]", depth);
      ImGui::SameLine();
      ImGui::Text("%s= [%d]", opNameShort[(int)currOp], depth+1);

      ImGui::Unindent();

    }
  }

  ImGui::EndTable();

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
  ImGui::TextWrapped("Conditions are on individual rows. The operator is applied between the current and previous condition."); ImGui::Spacing();
  ImGui::TextWrapped("Each line can be indented to increase it's depth (which adds parenthesis) and to create more complicated expressions."); ImGui::Spacing();
  ImGui::TextWrapped("Difference in the depth between the condition lines defines how many paretheses are opened or closed. One way to think about this is that, the more you increase the depth, the more the line will be associated with the previous line."); ImGui::Spacing();

  ImGui::Separator();
  ImGui::TextWrapped("The indentation UI can be made in many different ways. Here are two examples, using a drop down or buttons."); ImGui::Spacing();
  if (ImGui::RadioButton("Use depth drop down", useDepthDropdown)) useDepthDropdown = true;
  if (ImGui::RadioButton("Use depth buttons", !useDepthDropdown)) useDepthDropdown = false;

  ImGui::Separator();
  ImGui::TextWrapped("The basic algorithm does not support commonly used operator precedence (ANDs are calculate before ORs), which can add confusion when using same operators in one block"); ImGui::Spacing();
  ImGui::TextWrapped("Operator precedence can be implemented by adding more parenthesis. The option below enables that and also displays the extra parenthesis."); ImGui::Spacing();
  if (ImGui::Checkbox("Use precedence", &usePrecedence))
  {
    if (usePrecedence)
      guideOperatorUsage = false;
  }
  ImGui::Spacing();
  ImGui::TextWrapped("Even with precedence the result of mixing operators can be unexpected. An alternative is to guide in the UI to use same operators inside same sub-expression"); ImGui::Spacing();
  if (ImGui::Checkbox("Guide operator usage", &guideOperatorUsage))
  {
    if (guideOperatorUsage)
      usePrecedence = false;
  }

  ImGui::Separator();
  ImGui::TextWrapped("When the expression is used in a game setting, calculating the values of each row can be expensive (e.g. requires virtual function calls to other systems). Short circuiting can be used to skip these function calls."); ImGui::Spacing();
  ImGui::Checkbox("Use short-circuit", &useShortCircuit);

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

static void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}

int init_gl()
{
  
  if( !glfwInit() )
  {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return 1;
  }

	glfwSetErrorCallback(errorcb);

  //glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  // Open a window and create its OpenGL context
  g_width = canvas_get_width();
  g_height = canvas_get_height();

  g_window = glfwCreateWindow(g_width, g_height, "WebGui Demo", NULL, NULL);
  if( g_window == NULL )
  {
      fprintf( stderr, "Failed to open GLFW window.\n" );
      glfwTerminate();
      return -1;
  }
  glfwMakeContextCurrent(g_window); // Initialize GLEW


    int w, h;
    int fbw, fbh;
    glfwGetWindowSize(g_window, &w, &h);
    glfwGetFramebufferSize(g_window, &fbw, &fbh);
    printf("GLFW: Win: %dx%d  FB:%dx%d\n", w,h, fbw, fbh);


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

//#ifdef __EMSCRIPTEN__
//  resizeCanvas();
//#endif

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
