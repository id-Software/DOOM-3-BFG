// GL function stubs for cross-compilation
void GL_SelectTexture( int unit );
void GL_State( unsigned long long state, bool force );
void GL_Color( float r, float g, float b, float a );
void GL_SetDefaultState();
void GL_Cull( int cull );
void GL_Viewport( int x, int y, int w, int h );
void GL_Scissor( int x, int y, int w, int h );

void GL_SelectTexture( int unit ) {
    (void)unit;
}
void GL_State( unsigned long long state, bool force ) {
    (void)state; (void)force;
}
void GL_Color( float r, float g, float b, float a ) {
    (void)r; (void)g; (void)b; (void)a;
}
void GL_SetDefaultState() {}
void GL_Cull( int cull ) { (void)cull; }
void GL_Viewport( int x, int y, int w, int h ) { (void)x; (void)y; (void)w; (void)h; }
void GL_Scissor( int x, int y, int w, int h ) { (void)x; (void)y; (void)w; (void)h; }
