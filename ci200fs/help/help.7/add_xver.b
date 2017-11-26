
> Section 7 - add_xverb
>
> add_xverb(action,func)
>
> string action,func;
>
> add_xverb adds an xverb to the list of verbs on the object executing the
> system call. If add_xverb is called on the prototype, all the prototype's
> children will have the xverb defined for them.
>
> xverbs differ from regular verbs in that no space has to appear after
> the xverb's name in the player's command for the xverb to be recognized
> by the parser.
>
> xverb returns 0 on success, 1 on failure.
>
> See Also: add_verb(7), command(7), next_verb(7), remove_xverb(7),
>           set_interactive(7), set_localverbs(7)

