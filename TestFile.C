/*
 * REGOPT_COMPARE.C
 * Runtime regression for the TMS9900 R5-R8 expression-save optimisation.
 * Compile unchanged with both the original and patched Small-C/Plus.
 * Expected runtime output: REGOPT PASS
 */

int ga[8];
int side;


bump(x)
int x;
{
    side = side + 1;
    return x + 7;
}

/* Five nested saves: patched code should reach R8 and spill the fifth. */
nest(a,b,c,d,e,f)
int a,b,c,d,e,f;
{
    int x,y,z;

    x = a + (b + (c + (d + (e + f))));
    y = (a + b) + ((c + d) + (e + f));
    z = (a - b) + ((c - d) + (e - f));

    return x + y + z;
}

/* Local-variable loads inside nested saves exercise SP-offset rewriting. */
localmix(a,b,c,d,e,f)
int a,b,c,d,e,f;
{
    int p,q,r,s,t,u,v;

    p = a + 10;
    q = b + 20;
    r = c + 30;
    s = d + 40;
    t = e + 50;
    u = f + 60;

    v = p + (q + (r + (s + (t + u))));
    return v + p - u;
}

/* A live left operand crosses bump(): this save MUST remain on the stack. */
calltest(a,b,c)
int a,b,c;
{
    int x,y;

    side = 0;
    x = a + bump(b + c);
    y = bump(a + b) + c;

    return x + y;
}

/* Array lvalues plus local post-increment/decrement. */
arraytest(a,b,c)
int a,b,c;
{
    int i,x;

    i = 2;
    ga[0] = a;
    ga[1] = b;
    ga[2] = c;
    ga[3] = a + b;

    ga[i] = ga[i] + a + (b + c);
    x = ga[i];

    i++;
    ga[i] = x + i;
    i--;
    ga[i] = ga[i] + 1;

    return ga[0] + ga[1] + ga[2] + ga[3] + i;
}

/* while(i--) exercises the lvalue save/restore in a control-flow context. */
looptest(n)
int n;
{
    int i,sum;

    i = n;
    sum = 0;
    while (i--) {
        ga[i] = i + 10;
        sum = sum + ga[i];
    }

    return sum + i;
}

/* These outer saves cross branches and therefore must not be registerised. */
flowtest(a,b,c,d)
int a,b,c,d;
{
    int x,y,z;

    x = a + (b ? c : d);
    y = a + (b && c);
    z = a + (b || d);

    return x + y + z;
}

/* Local pointer post-step plus indirect loads/stores. */
ptrtest(a,b,c)
int a,b,c;
{
    int *p;
    int x;

    ga[0] = a;
    ga[1] = b;
    ga[2] = c;

    p = ga;
    x = *p;
    p++;
    x = x + *p;
    p++;
    x = x + *p;
    p--;
    *p = *p + 4;

    return x + *p;
}

main()
{
    int r;

    r = nest(1,2,3,4,5,6);
    if (r != 39) {
        puts("FAIL NEST");
        return 1;
    }

    r = localmix(1,2,3,4,5,6);
    if (r != 176) {
        puts("FAIL LOCAL");
        return 2;
    }

    r = calltest(10,20,30);
    if (r != 134) {
        puts("FAIL CALL VALUE");
        return 3;
    }
    if (side != 2) {
        puts("FAIL CALL SIDE");
        return 4;
    }

    r = arraytest(3,5,7);
    if (r != 58) {
        puts("FAIL ARRAY");
        return 5;
    }

    r = looptest(4);
    if (r != 45) {
        puts("FAIL LOOP");
        return 6;
    }

    r = flowtest(10,1,3,7);
    if (r != 35) {
        puts("FAIL FLOW TRUE");
        return 7;
    }

    r = flowtest(10,0,3,7);
    if (r != 38) {
        puts("FAIL FLOW FALSE");
        return 8;
    }

    r = ptrtest(2,4,6);
    if (r != 20) {
        puts("FAIL POINTER");
        return 9;
    }

    puts("REGOPT PASS");
    return 0;
}
