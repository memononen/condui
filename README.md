![Screenshot](/img/screenshot.png)

# Condition UI example in Dear ImGui

- **Live [Demo](https://memononen.github.io/condui/web/index.html)**

This is an example of a simple boolean expression editor and evaluation..

Conditions are on individual rows. The operator is applied between the current and previous condition, no operator precedence (there's automatic way to add extra indentation to have precedence, though).

```
if  IsDog 
or  IsCat
and IsHungry
```

Each line can be indented to add parenthesis and to create more complicated expressions.

```
if  (IsDog 
    or  IsCat)
and (IsHungry
    or  IsThirsty)
```

Difference in the identation between the condition lines defines how many paretheses are opened or closed. One way to think about this is that, the more you indent, the more the line will be associated with the previous line.

```
if  (   (IsDog 
        or  IsCat)
    and IsPet)
and (IsNearHome
    and (IsHungry
        or  IsThirsty))
```

## Motivation

Boolean expression UIs are needed in many applications, like search queries, or conditions in game logic (transitions, enter conditions, etc). Common approaches are to make such UI is to use a treeview using prefix notation or python style all/any containers, or some custom horizontal editor.

They all require complicated UI logic, and refactoring an expression usually becomes really hard. Depending on the UI framework, you may be required to redo the whole thing, if you want to encapsulate a branch.

Complex conditions are hard to express even in code, which is usually the most versatile format. Even there the congnitive load to decipher and refactor a complex expression is high.

Another thing to consider is that a lot of game engine editors are based on property grids, where each property is shown at separate line. The space for editing the conditions are usually very narrow, so a solution that expands vertically is needed.

This solutions aims to be quick to use, legible, vertical, and user proof. Mouse clicks can be minimized in the UI to make changing the expressions quick. Indentation and parentheses both make the expressions easy to read (there is similar style used on code too). This method creates always valid expressions, even if it might not be correct (e.g. during refactoring).

## Operator precedence

Operator precedence is hard. Some recent languages have opted to make it an error if different operators are used in same sub-expression. The basic version this code does not do any operator precedence, but there's option to automatically add parenthesis to achieve that. I think it is more confusing to use and adds quite a bit more complexity.

I experimented with with different options for the UI to maybe automatically adjust the operators at the same sub-expression
to match. In many cases this led more confusing experience. In the end the solution that felt least resticting was to add a
warning when an expression has different operators and a button to resolve the conflict. 

---

MIT license

Code based on [WebGui](https://github.com/jnmaloney/WebGui) sample project.
