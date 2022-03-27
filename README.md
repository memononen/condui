![Screenshot](/img/screenshot.png)

- [Demo](https://memononen.github.io/condui/imgui.html)

# Condition UI example in Dear ImGui

This is an example of a simple boolean expression editor and evaluation..

Conditions are on individual rows. The operator is applied between the current and previous condition, no operator precedence.

Each line can be indented to add parenthesis and to create more complicated expressions.

Difference in the identation between the condition lines defines how many paretheses are opened or closed. One way to think about this is that, the more you indent, the more the line will be associated with the previous line.

## Motivation

Boolean expression UIs are needed in many applications, like search queries, or conditions in game logic (transitions, enter conditions, etc). Common approaches are to make such UI is to use a treeview using prefix notation or python style all/any containers, or some custom horizontal editor.

They all require complicated UI logic, and refactoring an expression usually becomes really hard. Depending on the UI framework, you may be required to redo the whole thing, if you want to encapsulate a branch.

Complex conditions are hard to express even in code, which is usually the most versatile format. Even there the congnitive load to decipher and refactor a complex expression is high.

Another thing to consider is that a lot of game engine editors are based on property grids, where each property is shown at separate line. The space for editing the conditions are usually very narrow, so a solution that expands vertically is needed.

This solutions aims to be quick to use, legible, vertical, and user proof. Mouse clicks can be minimized in the UI to make changing the expressions quick. Indentation and parentheses both make the expressions easy to read (there is similar style used on code too). This method creates always valid expressions, even if it might not be correct (e.g. during refactoring).

---

MIT license

Code based on [WebGui](https://github.com/jnmaloney/WebGui) sample project.