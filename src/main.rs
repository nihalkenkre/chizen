#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(unused_variables)]
#![allow(unused_assignments)]

use std::error::Error;

mod vk;

use winit::{
    application::ApplicationHandler, dpi::PhysicalSize, event::WindowEvent, event_loop::EventLoop,
    raw_window_handle::HasWindowHandle, window::Window,
};

#[derive(Default)]
pub struct Application {
    window: Option<Window>,
    vk: vk::Vk,
}

impl ApplicationHandler for Application {
    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let window_size = PhysicalSize::new(1280, 720);
        self.window = Some(
            event_loop
                .create_window(
                    Window::default_attributes()
                        .with_max_inner_size(window_size)
                        .with_min_inner_size(window_size),
                )
                .unwrap(),
        );

        self.vk = vk::Vk::init(
            self.window
                .as_ref()
                .unwrap()
                .window_handle()
                .unwrap()
                .as_raw(),
        );
    }

    fn window_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        window_id: winit::window::WindowId,
        event: winit::event::WindowEvent,
    ) {
        match event {
            WindowEvent::CursorMoved {
                device_id,
                position,
            } => {}
            WindowEvent::RedrawRequested => {
                self.vk.render();
                self.window.as_ref().unwrap().request_redraw();
            }
            WindowEvent::CloseRequested => event_loop.exit(),
            _ => (),
        }
    }

    fn new_events(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        cause: winit::event::StartCause,
    ) {
        let _ = (event_loop, cause);
    }

    fn user_event(&mut self, event_loop: &winit::event_loop::ActiveEventLoop, event: ()) {
        let _ = (event_loop, event);
    }

    fn device_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        device_id: winit::event::DeviceId,
        event: winit::event::DeviceEvent,
    ) {
        let _ = (event_loop, device_id, event);
    }

    fn about_to_wait(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn suspended(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn exiting(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn memory_warning(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    let cmd_args: Vec<String> = std::env::args().collect();

    println!("Hello");

    if cmd_args.len() > 0 {
        println!("{:?}", cmd_args);
    } else {
        let event_loop = EventLoop::new()?;
        event_loop.set_control_flow(winit::event_loop::ControlFlow::Wait);
        let mut app = Application::default();
        event_loop.run_app(&mut app)?;

        // println!("Press any key to continue...");
        // let mut buf = String::new();
        // std::io::stdin()
        //     .read_line(&mut buf)
        //     .expect("failed to continue");
    }
    println!("Bye");

    Ok(())
}
