
> Section 7 - next_verb
>
> next_verb(obj,verb)
>
> object obj;
> string verb;
>
> Returns a string containing the next verb name defined on object obj,
> after verb.  If verb is NULL, the first verb name listed on object obj
> is returned. If no further verbs are defined on the object, NULL is
> returned.
>
> The last verb added to an object is always the first verb listed on it.
>
> NOTE: This will not list verbs defined on the prototype object if obj
>       is not the prototype itself.
>
> See Also: add_verb(7), add_xverb(7)

