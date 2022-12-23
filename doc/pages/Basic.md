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

### The engine {#doc-script-engine}

Expressions/scripts are evaluated by the plugin using an **evaluation engine**. An **engine instance** is a complete and isolated environment that can run
JavaScript scripts, like a **mini Web browser** or a continuously-running Node.js application
(it's not either of those things, but that's the best analogy I have for now).

**An engine instance can persist for the life of the plugin** (that is, it exists as long as the plugin keeps running).
* This is an important distinction vs., say, running a Node.js script from a Touch Portal action (using Run an Application or Run a batch file actions, for example).
  In the latter case the script has no way of saving information (data) between executions (not easily, anyway), plus every time it starts it will need to re-create
  it's complete environment, including starting node[.exe] itself and so on. A persistent environment like this plugin provides has no startup "cost" after the initial
  creation, and allows for easily saving  data, or "state" (lower case!), between runs.
  It's more like a Web browser that stays open and can re-run any loaded scripts on demand w/out restarting the whole thing.

### Engine instances  {#doc-engine-instance-types}

The plugin can **run multiple engine instances** at the same time. (Don't worry, this is not like running a bunch of bloated Firefox browsers...
each instance only uses a few KB of memory :-) ).
* There is always one **Shared Instance** of an engine in which any expression or script can run and it persists for the life of the plugin
  (its environment/state can be reset manually if needed).
* Any expression/script you want to run via this plugin's actions can optionally use its own **Private Instance** of an engine.
  These engines persist after initial creation, independently from each other, for as long as the plugin keeps running or you delete them intentionally.

Other differences between Shared and Private engine instances are described on the next page about the plugin's actions.

### Named instances  {#doc-named-state-instances}

Almost all the actions described below (with one exception) revolve around what I call the **State Name** in the actions and other places.
* Because once activated they will create a new Touch Portal State with the name you type in (or update it, if it already exists).
* These can also be thought of as **named expression/script instances** (for lack of a better term).
* Each of these named instances can use either the Shared or a Private evaluation engine. So effectively a State Name for an action using a Private
  engine also becomes the engine instance's name.
* These names are referenced in the plugin's other action(s), such as where you can delete one or reset its engine environment
  (described on the [actions page](@ref actions_plugin-action)).

### Important:
To get any kind of result back from the plugin to Touch Portal the plugin creates Touch Portal States dynamically **once you tell it to**.

Meaning that to be able to select the new State from Touch Portal's interface you have to first send one of the plugin's actions which creates
that State in the first place (with a name of your choice, as you'll see on the next page). It's a bit of a [catch-22](https://en.wikipedia.org/wiki/Catch-22_(logic))
and this is sometimes a point of confusion.

So in general the procedure for using any of these results is:
1. First set up one of the plugin's actions (which creates a new State); It doesn't have to be your final expression/script,
   just something valid to create the initial State with.
2. Then activate this action once somehow (on button press or whatever) -- assuming there are no errors in the expression/script (another good reason to start simple),
   this will crate the new State in Touch Portal.
3. Now go back and finish setting up your button/event once the new State has been created and will be available in the various places you can select States from.

Alternately:
1. Create the Touch Portal State first with a simple JavaScript expression: `TP.stateCreate("dsep.MyNewStateName", "Description of My New State")`
2. Using the ["one-time script"](@ref plugin_actions_one-time-script) actin is perfect for this.

OK, enough of that, let's get to the Actions!

<span class="next_section_button">
Read Next: [Plugin Actions](Actions.md)
</span>
