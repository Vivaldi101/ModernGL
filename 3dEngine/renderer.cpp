
enum 
{
	RCMD_CLEAR = 1,
	RCMD_END_OF_CMDS,
	RCMD_DRAW_BASIS2D,
    RCMD_DRAW_RECTANGLE,
    RCMD_DRAW_BITMAP,
	RCMD_SWAP_BUFFERS,
};

#define GetRenderCommandData(cmd) (cmd *)((byte*)(renderBuffer) + sizeof(((Draw*)renderBuffer)->id));
#define PushRenderCommandType(buffer, type) (Draw *)(PushRenderCommandType_)((buffer), sizeof(type) + sizeof(Draw))

// TODO: Pass transforms.
// TODO: Change data to memory.
#define RenderCommand(name) \
function const void * \
name(Bitmap *outputTarget, const void *renderBuffer)

struct Draw
{
	u32 id; // Important! Do not put any field before or after id.
	union Commands
	{
		struct DrawBasis2dCmd
		{
			V4 color;
			V2 origin;
			V2 xAxis;
			V2 yAxis;
			V2 basisPoint;
			f32 axisScale;
		};

		struct DrawRectangleCmd
		{
			V4 color;
			V2 min;
			V2 max;
		};

		struct DrawBitmapCmd
		{
			Bitmap *bitmap;
			f32 originX;
			f32 originY;
			f32 alignX;
			f32 alignY;
		};

		struct SwapBuffersCmd
		{
			Rectangle2 drawRegion;
			void* textureMemory;
			HWND windowHandle;
			//BITMAPINFO *bmpInfo; // TODO: For software rendering
		};
	};
};

function void
DrawTextureToRegion(void* textureMemory, Rectangle2 drawRegion)
{
	s32 width = drawRegion.right - drawRegion.left;
	s32 height = drawRegion.top - drawRegion.bottom;

	Defer(width >= 2 && height >= 2, true)
	{
		// Test margin.
		int pixelMargin = 0;
		glViewport(0 + pixelMargin, 0 + pixelMargin, width - pixelMargin*1, height - pixelMargin*1);
		{
			// Texturing.
			// TODO: Refactor.
			persistent b32 initialize = false;
			u32 textureHandle = 0;
			if(!initialize)
			{
				glGenTextures(1, &textureHandle);
				initialize = true;
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, textureMemory);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glEnable(GL_TEXTURE_2D);
		}
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		{
			f32 a = 2.0f / (width - 1.0f);
			f32 b = 2.0f / (height - 1.0f);
			f32 projectionMatrix[] =
			{
				a, 0, 0, 0,
				0, b, 0, 0,
				0, 0, 1, 0,
			   -1, -1, 0, 1,
			};
			glLoadMatrixf(projectionMatrix);
		}

		//glDisable(GL_TEXTURE_2D);
		glBegin(GL_TRIANGLES);
		{
			f32 left = 0.0f;
			f32 top = 0.0f;

			f32 right = width - 1.0f;
			f32 bottom = height - 1.0f;

			// Upper left triangle.
			//glColor3f(1.0f, 0.0f, 0.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2f(left, top);

			//glColor3f(0.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(left, bottom);

			//glColor3f(0.0f, 0.0f, 1.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(right, top);

			// Lower right triangle.
			//glColor3f(0.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(left, bottom);

			//glColor3f(1.0f, 0.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2f(right, bottom);

			//glColor3f(0.0f, 0.0f, 1.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(right, top);
		}
		glEnd();
	}
}

// Assumes: Color byte order in memory is b[0]==B b[1]==G b[2]==R b[3]==A: BGRA
function void
DrawBitmapToOutputTarget(Bitmap *outputTarget, Bitmap *bitmap, f32 roriginX, f32 roriginY, f32 alignX, f32 alignY)
{
    void *outputTargetPixels = outputTarget->memory;
    void *bitmapPixels = bitmap->memory;
    s32 outputTargetPitch = outputTarget->pitch;
    s32 bitmapPitch = bitmap->pitch;
    s32 outputTargetWidth = outputTarget->width;
    s32 outputTargetHeight = outputTarget->height;
    s32 outputTargetBpp = outputTargetPitch / outputTargetWidth;
    s32 bitmapHeight = bitmap->height;
    s32 bitmapWidth = bitmap->width;
    s32 bitmapBpp = bitmapPitch / bitmapWidth;
    
    s32 minX = RoundReal32ToS32(roriginX - alignX);
    s32 minY = RoundReal32ToS32(roriginY - alignY);
    
    s32 maxX = minX + bitmapWidth;
    s32 maxY = minY + bitmapHeight;
    
    s32 bitmapOffsetX = 0;
    s32 bitmapOffsetY = 0;
    
    if(bitmapBpp < 0)
    {
        bitmapBpp = -bitmapBpp;
    }
    if(outputTargetBpp < 0)
    {
        outputTargetBpp = -outputTargetBpp;
    }
    
    // Clip to output target.
    if(minX < 0)
    {
        bitmapOffsetX = -minX;
        minX = 0;
    }
    if(minY < 0)
    {
        bitmapOffsetY = -minY;
        minY = 0;
    }
    
    if(maxX > outputTargetWidth)
    {
        maxX = outputTargetWidth;
    }
    if(maxY > outputTargetHeight)
    {
        maxY = outputTargetHeight;
    }
    
    byte *bitmapRow = (byte *)bitmapPixels + (bitmapOffsetY*bitmapPitch) + (bitmapOffsetX*bitmapBpp);
    byte *outputTargetRow = (byte *)outputTargetPixels + (minY*outputTargetPitch) + (minX*outputTargetBpp);
    
    for(s32 y = minY; y < maxY; y++)
    {
        u32 *bitmapPixel = (u32 *)bitmapRow;
        u32 *outputTargetPixel = (u32 *)outputTargetRow;
        for(s32 x = minX; x < maxX; x++)
        {
            f32 salpha = (f32)((*bitmapPixel >> 24) & 0xff)/255.0f;
            f32 sred = (f32)((*bitmapPixel >> 16) & 0xff);
            f32 sgreen = (f32)((*bitmapPixel >> 8) & 0xff);
            f32 sblue = (f32)((*bitmapPixel >> 0) & 0xff);
            V4 inputColor = {sred, sgreen, sblue, salpha};
            
            f32 dalpha = (f32)((*outputTargetPixel >> 24) & 0xff)/255.0f;
            f32 dred = (f32)((*outputTargetPixel >> 16) & 0xff);
            f32 dgreen = (f32)((*outputTargetPixel >> 8) & 0xff);
            f32 dblue = (f32)((*outputTargetPixel >> 0) & 0xff);
            V4 outputColor = {dred, dgreen, dblue, dalpha};
            
            V4 cfinal = (1.0f-inputColor.a)*outputColor + inputColor;
            //u32 calpha = (u32)(((1.0f-inputColor.a)*outputColor.a + inputColor.a)*255.0f + 0.5f);
            u32 calpha = (u32)((cfinal.a)*255.0f + 0.5f);
            u32 cred = (u32)(cfinal.r + 0.5f);
            u32 cgreen = (u32)(cfinal.g + 0.5f);
            u32 cblue = (u32)(cfinal.b + 0.5f);
            
            u32 color = (calpha << 24) | (cred << 16) | (cgreen << 8) | (cblue << 0);
            //u32 color = (255 << 24) | (cred << 16) | (cgreen << 8) | (cblue << 0);
            
            *outputTargetPixel = color;
            outputTargetPixel++;
            bitmapPixel++;
        }
        outputTargetRow += outputTargetPitch;
        bitmapRow += bitmapPitch;
    }
}

function void
DrawRectangleToOutputTarget(Bitmap *outputTarget, V2 origin, V2 xAxis, V2 yAxis, V4 color)
{
	// TODO: Line clip basis vector to output target.
	V2 min = origin;
	V2 max = min + xAxis + yAxis;

	// Clip to output target.
    if(min.x < 0)
    {
        min.x = 0;
    }
    if(min.y < 0)
    {
        min.y = 0;
    }

    if(max.x > outputTarget->width)
    {
        max.x = outputTarget->width;
    }
    if(max.y > outputTarget->height)
    {
        max.y = outputTarget->height;
    }

	s32 outputTargetWidth = outputTarget->width;
	s32 outputTargetHeight = outputTarget->height;
	s32 pitch = outputTarget->pitch;
    u32 packedColor = PackRGBA(color);
	byte *pixelMemory = Cast(byte*, outputTarget->memory);

	for(s32 y = 0;
		y < outputTargetHeight;
		y++)
    {
		u32 *pixels = Cast(u32*, pixelMemory);
        for(s32 x = 0;
			x < outputTargetWidth;
			x++)
        {
			V2 test = {(f32)x, (f32)y};

			// TODO: Fixed point?
			f32 edgeTest0 = Dot(test - min, -yAxis);			// Bottom.
			f32 edgeTest1 = Dot(test - (max - 1.0f), xAxis);	// Right.
			f32 edgeTest2 = Dot(test - (max - 1.0f), yAxis);	// Top.
			f32 edgeTest3 = Dot(test - min, -xAxis);			// Left.

			if (edgeTest0 < 0.0f &&
				edgeTest1 < 0.0f &&
				edgeTest2 < 0.0f &&
				edgeTest3 < 0.0f)
			{
				*pixels = packedColor;
			}
			pixels++;
        }
        pixelMemory += pitch;
    }
}

function void
DrawRectangleToOutputTarget(Bitmap *outputTarget, V2 min, V2 max, V4 color)
{
	// Actually have a rectangle to draw.
	Assert(min.x < max.x);
	Assert(min.y < max.y);

	s32 roundedMinX = RoundReal32ToS32(min.x);
    s32 roundedMinY = RoundReal32ToS32(min.y);
    s32 roundedMaxX = RoundReal32ToS32(max.x);
    s32 roundedMaxY = RoundReal32ToS32(max.y);
    
    u32 packedColor = PackRGBA(color);
    
	// Clip to output outputTarget.
    if(roundedMinX < 0)
    {
        roundedMinX = 0;
    }
    if(roundedMinY < 0)
    {
        roundedMinY = 0;
    }
    if(roundedMaxX > outputTarget->width)
    {
        roundedMaxX = outputTarget->width;
    }
    if(roundedMaxY > outputTarget->height)
    {
        roundedMaxY = outputTarget->height;
    }
    
	u32 pitch = outputTarget->pitch;
    byte *row = (byte *)outputTarget->memory + (roundedMinY*pitch) + (roundedMinX*BYTES_PER_PIXEL);
    
    for(s32 y = roundedMinY; y < roundedMaxY; y++)
    {
        u32 *pixel = (u32 *)row;
        for(s32 x = roundedMinX; x < roundedMaxX; x++)
        {
            *pixel++ = packedColor;
        }
        row += pitch;
    }
}

function void *
PushRenderCommandType_(PushBuffer *renderCommands, u32 byteCount) 
{
	byte *commandBuffer = (byte*)renderCommands->memory;
    
	// Leave room for the end of list command.
	if ((renderCommands->usedSize + byteCount + sizeof(u32)) > renderCommands->maxSize) 
    {
		// Drop the current command if overflowing.
		return 0;
	}
	renderCommands->usedSize += byteCount;
    
	return commandBuffer + renderCommands->usedSize - byteCount;
}

RenderCommand(DrawBasis2d)
{
    Draw::Commands::DrawBasis2dCmd *cmd = GetRenderCommandData(Draw::Commands::DrawBasis2dCmd);

	V2 origin = cmd->origin;

	V2 xAxis = cmd->xAxis * cmd->axisScale;
	V2 yAxis = cmd->yAxis * cmd->axisScale;

	V2 basisPoint = cmd->basisPoint;
	V4 basisColor = {1.0f, 1.0f, 1.0f, 1.0f};
	V2 xyMax = xAxis + yAxis;

	DrawRectangleToOutputTarget(outputTarget, origin, xAxis, yAxis, cmd->color);

	xAxis = Normalize(xAxis) * basisPoint.x;
	yAxis = Normalize(yAxis) * basisPoint.y;

	// Origin point.
	DrawRectangleToOutputTarget(outputTarget, origin, xAxis, yAxis, basisColor);

	// Max x & y point.
	DrawRectangleToOutputTarget(outputTarget, origin + xyMax - (xAxis + yAxis), xAxis, yAxis, basisColor);

    return Cast(const void *, cmd + 1);
}

RenderCommand(DrawRectangle)
{
    Draw::Commands::DrawRectangleCmd *cmd = GetRenderCommandData(Draw::Commands::DrawRectangleCmd);
    DrawRectangleToOutputTarget(outputTarget, cmd->min, cmd->max, cmd->color);
    
    return Cast(const void *, cmd + 1);
}

RenderCommand(DrawBitmap)
{
    Draw::Commands::DrawBitmapCmd *cmd = GetRenderCommandData(Draw::Commands::DrawBitmapCmd);
    DrawBitmapToOutputTarget(outputTarget, cmd->bitmap, cmd->originX, cmd->originY, cmd->alignX, cmd->alignY);
    
    return Cast(const void *, cmd + 1);
}

RenderCommand(SwapBuffers)
{
    Draw::Commands::SwapBuffersCmd *cmd = GetRenderCommandData(Draw::Commands::SwapBuffersCmd);

	DrawTextureToRegion(cmd->textureMemory, cmd->drawRegion);

	HDC contextHandle = SysGetWindowContextHandle(cmd->windowHandle);

	SwapBuffers(contextHandle);

	SysReleaseWindowContextHandle(cmd->windowHandle, contextHandle);
    
    return Cast(const void *, cmd + 1);
}

function void
PushEndDraw(Bitmap* activeTexture, PushBuffer *renderCommands, AppWindow* window)
{
	Draw *drawCmd = PushRenderCommandType(renderCommands, Draw::Commands::SwapBuffersCmd);
	drawCmd->id = RCMD_SWAP_BUFFERS;

	Draw::Commands::SwapBuffersCmd *cmd = (Draw::Commands::SwapBuffersCmd*)((byte*)drawCmd + sizeof(drawCmd->id));

	cmd->drawRegion = window->region;
	cmd->windowHandle = window->handle;
	cmd->textureMemory = activeTexture->memory;
}

function void
PushBasis2d(PushBuffer *renderCommands, V2 xAxis, V2 yAxis, V2 origin, V2 basisPoint, f32 axisScale, V4 color)
{
	Draw *drawCmd = PushRenderCommandType(renderCommands, Draw::Commands::DrawBasis2dCmd);
	drawCmd->id = RCMD_DRAW_BASIS2D;

    Draw::Commands::DrawBasis2dCmd *cmd = (Draw::Commands::DrawBasis2dCmd*)((byte*)drawCmd + sizeof(drawCmd->id));

	cmd->color = color;
	cmd->origin = origin;
	cmd->xAxis = xAxis;
	cmd->yAxis = yAxis;
	cmd->axisScale = axisScale;
	cmd->basisPoint = basisPoint;
}

function void
PushDrawRectangle(PushBuffer *renderCommands, V2 min, V2 max, V4 color)
{
    Draw *drawCmd = PushRenderCommandType(renderCommands, Draw);
	drawCmd->id = RCMD_DRAW_RECTANGLE;

    Draw::Commands::DrawRectangleCmd *cmd = (Draw::Commands::DrawRectangleCmd*)((byte*)drawCmd + sizeof(drawCmd->id));

    cmd->min = min;
    cmd->max = max;
    cmd->color = color;
}

function void
PushDrawRectangle(PushBuffer *renderCommands, f32 minX, f32 minY, f32 maxX, f32 maxY, V4 color)
{
    Draw *drawCmd = PushRenderCommandType(renderCommands, Draw::Commands::DrawRectangleCmd);
	drawCmd->id = RCMD_DRAW_RECTANGLE;

    Draw::Commands::DrawRectangleCmd *cmd = (Draw::Commands::DrawRectangleCmd*)((byte*)drawCmd + sizeof(drawCmd->id));

    cmd->min = MakeV2(minX, minY);
    cmd->max = MakeV2(maxX, maxY);
    cmd->color = color;
}

function void
PushDrawBitmap(PushBuffer *renderCommands, Bitmap *bitmap,
                      f32 roriginX, f32 roriginY,
                      f32 alignX, f32 alignY)
{
    Draw *drawCmd = PushRenderCommandType(renderCommands, Draw);
    drawCmd->id = RCMD_DRAW_BITMAP;

    Draw::Commands::DrawBitmapCmd *cmd = (Draw::Commands::DrawBitmapCmd*)((byte*)drawCmd + sizeof(drawCmd->id));

    cmd->originX = roriginX;
    cmd->originY = roriginY;
    cmd->alignX = alignX;
    cmd->alignY = alignY;
    cmd->bitmap = bitmap;
}


function void
ExecuteRenderBuffer(Bitmap *outputTarget, const void *data) 
{
    for (;;) 
    {
		// + 1 is for skipping the id field within the Draw structure.
		switch(Cast(Draw*, data)->id)
        {
			// Render buffer empty now.
            case RCMD_END_OF_CMDS:
            {
                return;
            } break;
			case RCMD_DRAW_BASIS2D:
			{
				data = DrawBasis2d(outputTarget, Cast(u32*, data));
			} break;
            case RCMD_DRAW_RECTANGLE:
            {
                data = DrawRectangle(outputTarget, Cast(u32*, data));
            } break;
            case RCMD_DRAW_BITMAP:
            {
                data = DrawBitmap(outputTarget, Cast(u32*, data));
            } break;
            case RCMD_SWAP_BUFFERS:
            {
                data = SwapBuffers(outputTarget, Cast(u32*, data));
            } break;
            default:
            {
                InvalidCodePath("Invalid render command");
            } break;
        }
    }
}

function void 
IssueRenderCommands(Bitmap* activeTexture, PushBuffer *renderCommands) 
{
	// Mark the end of render command buffer.
	*Cast(u32 *, (byte*)renderCommands->memory + renderCommands->usedSize) = RCMD_END_OF_CMDS;

    // TODO: Add custom textures to be used for the final blit.
	ExecuteRenderBuffer(activeTexture, renderCommands->memory);
    
    // Clear the render buffer at the end.
	renderCommands->usedSize = 0;
}
