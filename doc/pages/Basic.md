# Basic Principles {#plugin_basics}

[TOC]

## General Theory and Terminology

The plugin **evaluates JavaScript-compatible expressions**, from simple math to running (relatively) complex programs.

For the most part this is useful for getting information/data back to Touch Portal, for example to get the result of a math expression as soon as some
dynamic value (used in the expression) changes. Such results would be sent to Touch Portal as plug-in "States" (which I will refer to with a capital letter
to help avoid confusion with any other kind of state).

That's not an exclusive usage -- scripts can also be run that simply do something useful but do not return any result. _Or_ they may run
in the background and send Touch Portal State (capital!) updates when new data becomes available (such as watching a file for changes or downloading from a URL).
But more on that later.

### The Engine {#doc-script-engine}

Expressions/scripts are evaluated by the plugin using an **evaluation engine**. An **engine instance** is a complete and isolated environment that can run
JavaScript scripts, like a **mini Web browser** or a continuously-running Node.js application
(it's not either of those things, but that's the best analogy I have for now).

**An engine instance can persist for the life of the plugin** (that is, it exists as long as the plugin keeps running).
* This is an important distinction vs., say, running a Node.js script from a Touch Portal action (using Run an Application or Run a batch file actions, for example).
  In the latter case the script has no way of saving information (data) between executions (not easily, anyway), plus every time it starts it will need to re-create
  it's complete environment, including starting Node itself and so on. A persistent environment like this plugin provides has no startup "cost" after the initial
  creation, and allows for easily saving  data, or "state" (lower case!), between runs.
  It's more like a Web browser that stays open and can re-run any loaded scripts on demand w/out restarting the whole thing.

### Engine instances  {#doc-engine-instance-types}

The plugin can **run multiple engine instances** at the same time. (Don't worry, this is not like running a bunch of bloated Firefox browsers...
each instance only uses a few KB of memory :-) ).
* There is always one **Shared Instance** of an engine in which any expression or script can run and it persists for the life of the plugin
  (its environment/state can be reset manually if needed).
* Any expression/script you want to run via this plugin's actions can optionally use its own **Private Instance** of an engine.
  These engines persist after initial creation, independently from each other, for as long as the plugin keeps running or you delete them intentionally.
* Multiple expressions/scripts can also share a Private engine instance.

Other differences between Shared and Private engine instances are described on the next page about the plugin's actions.

### Script instances  {#doc-named-state-instances}

Each JavaScript expression initiated from a Touch Portal Action or Connector is referred-to as a **Script Instance**.
The actual actions/connectors are described in following pages, but they all share the same basic principles:
* Each Script Instance is defined by it's **Instance Name**. Multiple actual Touch Portal actions/connectors can specify the same Instance Name,
  but only one of these can be evaluated at a time.
* Each Instance can optionally create (or update) a Touch Portal State. The created State name, if any, will match the Instance Name.
* Each of these named instances can use either the Shared or a Private evaluation engine.
  * Effectively an Instance Name for an action using a Private engine also becomes the engine instance's name.
  * Once a named engine instance exists, any other Script Instance can also use it. So there could be multiple named Script instances,
    with different names, all using the same Private engine.
* These "Instance Names" are referenced in the plugin's other action(s), such as where you can delete one or reset its engine environment
  (described on a following page).


### Creating and Using States - Important
To get any kind of result back from the plugin to Touch Portal the plugin creates Touch Portal States dynamically **once you tell it to**.

Meaning that to be able to select the new State from Touch Portal's interface you have to first send one of the plugin's actions which creates
that State in the first place (with a name of your choice, as you'll see on the next page). It's a bit of a [catch-22](https://en.wikipedia.org/wiki/Catch-22_(logic))
and this is sometimes a point of confusion.

So in general the procedure for using any of these results is:
1. First set up one of the plugin's actions (which creates a new State); It doesn't have to be your final expression/script,
   just something valid to create the initial State with.
2. Then activate this action once somehow (on button press, for example) -- assuming there are no errors in the expression/script (another good reason to start simple),
   this will crate the new State in Touch Portal.
3. Now go back and finish setting up your button/event once the new State has been created and will be available in the various places you can select States from.

#### Touch Portal State ID vs. Name

As a "final" detail, note that __States in Touch Portal are tracked and uniquely identified by an ID__, not the Name displayed in most places. You can see these
actual IDs when selecting a State or Value from the `+` buttons next to text entry fields and such... they're the part in `${value:...}` macros after the "value:" part
and before the closing "}".

When you create a new State/Instance using this plugin's actions (or connectors), the State ID is generated by adding a `dsep.` prefix to the Instance Name you specify.
This is to help make sure the IDs are unique and not going to overwrite/compete with any other States.

So for example if you run a Script Instance named "My New State Name" and have it create a State, the State's ID will actually be
`dsep.My New State Name`. When you select that State using those `+` buttons, the "macro" Touch Portal inserts would look like
`${value:dsep.My New State Name}`

--------

OK, enough of that, let's get to the Actions!

<span class="next_section_button">
Read Next: [Plugin Actions](Actions.md)
</span>
