#include <texture/vsx_texture.h>
#include <texture/buffer/vsx_texture_buffer_color_depth.h>

class module_texture_render_surface_color_depth_buffer : public vsx_module
{
  // in
  vsx_module_param_render* render_in;
  vsx_module_param_int* texture_size_in;
  vsx_module_param_int* float_texture_in;
  vsx_module_param_int* alpha_channel_in;
  vsx_module_param_int* multisample_in;
  vsx_module_param_int* min_mag_filter_in;
  vsx_module_param_texture* depth_buffer_in;
  vsx_module_param_float* size_x_in;
  vsx_module_param_float* size_y_in;

  // out
  vsx_module_param_texture* texture_out;
  vsx_module_param_texture* depth_buffer_out;

  // internal
  int res_x, res_y;
  int dbuff;
  int tex_size_enum;
  vsx_texture<>* texture = 0x0;
  vsx_texture<> depth_buffer_texture;
  vsx_texture_buffer_color_depth buffer;

  int float_texture_cache;
  int alpha_channel_cache;
  int multisample_cache;
  int min_mag_filter_cache;

  GLuint depth_buffer_in_int;

  vsx_gl_state* gl_state;

public:

  void module_info(vsx_module_specification* info)
  {
    info->identifier = "texture;buffers;render_surface_color_depth_buffer";

    info->description =
      "This module captures rendering to a texture\n"
      "- color buffer RGBA\n"
      "- depth buffer 24-bit\n"
      "\n"
      "Everything you render connected to this module\n"
      "is stored in the texture and can be reused when\n"
      "rendering other objects.\n"
      "This module can also output depth buffer for\n"
      "Another identical module to use (shared depth\n"
      "buffer, or it can (via the input) use another\n"
      "module's depth buffer.\n"
      "It supports floating point textures\n"
      "and optional alpha channel.\n"
      "Dynamic textures can be very useful!";

    info->in_param_spec =
      "render_in:render,"
      "depth_buffer:texture,"
      "options:complex"
      "{"
        "texture_size:enum?"
          "2048x2048|"
          "1024x1024|"
          "512x512|"
          "256x256|"
          "128x128|"
          "64x64|"
          "32x32|"
          "16x16|"
          "8x8|"
          "4x4|"
          "VIEWPORT_SIZE|"
          "VIEWPORT_SIZE_DIV_2|"
          "VIEWPORT_SIZE_DIV_4|"
          "VIEWPORT_SIZEx2|"
          "VIEWPORT_SIZEx4|"
          "CUSTOM_SIZE&nc=1,"
        "size_x:float,"
        "size_y:float,"
        "float_texture:enum?no|yes&nc=1,"
        "alpha_channel:enum?no|yes&nc=1,"
        "multisample:enum?no|yes&nc=1,"
        "min_mag_filter:enum?nearest|linear&nc=1"
      "}"
    ;
    info->out_param_spec =
      "color_buffer:texture,"
      "depth_buffer:texture"
    ;
    info->component_class = "texture";
  }

  void declare_params(vsx_module_param_list& in_parameters, vsx_module_param_list& out_parameters) {
    render_in = (vsx_module_param_render*)in_parameters.create(VSX_MODULE_PARAM_ID_RENDER, "render_in",false,false);
    render_in->run_activate_offscreen = true;
    res_x = 512;

    float_texture_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "float_texture");
    float_texture_in->set(0);
    float_texture_cache = 0;

    alpha_channel_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "alpha_channel");
    alpha_channel_in->set(1);
    alpha_channel_cache = 1;

    multisample_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "multisample");
    multisample_in->set(0);
    multisample_cache = 0;

    min_mag_filter_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "min_mag_filter");
    min_mag_filter_in->set(1); // linear
    min_mag_filter_cache = 1;

    texture_size_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "texture_size");
    texture_size_in->set(2);

    size_x_in = (vsx_module_param_float*)in_parameters.create(VSX_MODULE_PARAM_ID_FLOAT, "size_x");
    size_x_in->set(32.0);

    size_y_in = (vsx_module_param_float*)in_parameters.create(VSX_MODULE_PARAM_ID_FLOAT, "size_y");
    size_y_in->set(32.0);

    tex_size_enum = -1;

    texture_out = (vsx_module_param_texture*)out_parameters.create(VSX_MODULE_PARAM_ID_TEXTURE,"color_buffer");

    depth_buffer_out = (vsx_module_param_texture*)out_parameters.create(VSX_MODULE_PARAM_ID_TEXTURE,"depth_buffer");
    depth_buffer_in = (vsx_module_param_texture*)in_parameters.create(VSX_MODULE_PARAM_ID_TEXTURE,"depth_buffer");
    depth_buffer_in_int = 0;

    gl_state = vsx_gl_state::get_instance();
  }

  bool can_run()
  {
    return vsx_texture_buffer_base::has_buffer_support();
  }

  void start()
  {
  }

  bool activate_offscreen()
  {
    bool rebuild = false;

    if (alpha_channel_in->get() != alpha_channel_cache)
    {
      alpha_channel_cache = alpha_channel_in->get();
      rebuild = true;
    }

    if (float_texture_in->get() != float_texture_cache)
    {
      float_texture_cache = float_texture_in->get();
      rebuild = true;
    }

    if (multisample_in->get() != multisample_cache)
    {
      multisample_cache = multisample_in->get();
      rebuild = true;
    }

    if (min_mag_filter_in->get() != min_mag_filter_cache)
    {
      min_mag_filter_cache = min_mag_filter_in->get();
      rebuild = true;
    }

    if (depth_buffer_in->connected && depth_buffer_in->valid)
    {
      vsx_texture<>* depth_in = depth_buffer_in->get();
      if ( depth_buffer_in_int != depth_in->texture->gl_id )
      {
        depth_buffer_in_int = depth_in->texture->gl_id;
        rebuild = true;
      }
    } else
    {
      if (0 != depth_buffer_in_int)
        rebuild = true;
      depth_buffer_in_int = 0;
    }

    if (texture_size_in->get() >= 10)
    {
      int t_res_x = gl_state->viewport_get_width();
      int t_res_y = gl_state->viewport_get_height();

      if (texture_size_in->get() == 10)
        if (t_res_x != res_x || t_res_y != res_y)
          rebuild = true;

      if (texture_size_in->get() == 11)
        if (t_res_x / 2 != res_x || t_res_y / 2 != res_y)
          rebuild = true;

      if (texture_size_in->get() == 12)
        if (t_res_x / 4 != res_x || t_res_y / 4 != res_y)
          rebuild = true;

      if (texture_size_in->get() == 13)
        if (t_res_x * 2 != res_x || t_res_y * 2 != res_y)
          rebuild = true;

      if (texture_size_in->get() == 14)
        if (t_res_x * 4 != res_x || t_res_y * 4 != res_y)
          rebuild = true;
    }


    if (texture_size_in->get() == 15)
    {
      tex_size_enum = texture_size_in->get();

      float i_size_x = size_x_in->get();
      float i_size_y = size_y_in->get();

      if (i_size_x < 1.0)
        i_size_x = 1.0;

      if (i_size_y < 1.0)
        i_size_y = 1.0;

      if ((int)i_size_x != res_x || (int)i_size_y != res_y)
      {
        res_x = (int)i_size_x;
        res_y = (int)i_size_y;
        rebuild = true;
      }
    }


    if (texture_size_in->get() != tex_size_enum || rebuild)
    {
      tex_size_enum = texture_size_in->get();

      switch (tex_size_enum) {
        case 0: res_y = res_x = 2048; break;
        case 1: res_y = res_x = 1024; break;
        case 2: res_y = res_x = 512; break;
        case 3: res_y = res_x = 256; break;
        case 4: res_y = res_x = 128; break;
        case 5: res_y = res_x = 64; break;
        case 6: res_y = res_x = 32; break;
        case 7: res_y = res_x = 16; break;
        case 8: res_y = res_x = 8; break;
        case 9: res_y = res_x = 4; break;
        case 10:
          res_x = gl_state->viewport_get_width();
          res_y = gl_state->viewport_get_height();
        break;
        case 11:
          res_x = gl_state->viewport_get_width() * 0.5;
          res_y = gl_state->viewport_get_height() * 0.5;
        break;
        case 12:
          res_x = gl_state->viewport_get_width() * 0.25;
          res_y = gl_state->viewport_get_height() * 0.25;
        break;
        case 13:
          res_x = gl_state->viewport_get_width() * 2.0;
          res_y = gl_state->viewport_get_height() * 2.0;
        break;
        case 14:
          res_x = gl_state->viewport_get_width() * 4.0;
          res_y = gl_state->viewport_get_height() * 4.0;
        break;
      };

      if ( depth_buffer_in_int != 0 )
      {
        vsx_texture<>* depth_in = depth_buffer_in->get();
        if (
            depth_in->texture->bitmap->width != (unsigned int)res_x ||
            depth_in->texture->bitmap->height != (unsigned int)res_y
          )
        {
          res_x = depth_in->texture->bitmap->width;
          res_y = depth_in->texture->bitmap->height;
        }
      }

      if (!texture)
        texture = new vsx_texture<>;

      buffer.reinit
      (
        texture,
        res_x,
        res_y,

        float_texture_cache,
        alpha_channel_cache,
        multisample_cache,
        min_mag_filter_cache,
        depth_buffer_in_int
      );

      depth_buffer_texture.texture->gl_id = buffer.get_depth_buffer_handle();
      depth_buffer_texture.texture->gl_type = GL_TEXTURE_2D;
      depth_buffer_texture.texture->uploaded_to_gl = true;

      depth_buffer_texture.texture->bitmap->width = res_x;
      depth_buffer_texture.texture->bitmap->height = res_y;

      depth_buffer_out->set(&depth_buffer_texture);

      texture_out->set(texture);
    }

    buffer.begin_capture_to_buffer();

    glUseProgram(0);

    loading_done = true;
    return true;
  }

  void deactivate_offscreen()
  {
    if (texture)
      buffer.end_capture_to_buffer();
    ((vsx_module_param_texture*)texture_out)->set(texture);
  }

  void stop()
  {
    if (!texture)
      return;

    buffer.deinit( texture );
    delete texture;
    texture = 0;

    // make sure it's rebuilt
    tex_size_enum = -1;
  }

  void on_delete()
  {
    stop();
  }

  ~module_texture_render_surface_color_depth_buffer()
  {
    if (texture)
      delete texture;
  }

};

